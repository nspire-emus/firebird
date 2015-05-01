/*
 * How the ARM translation works:
 * translation_enter in asmcode_arm.S finds the entry point and jumps to it.
 * Some virtual registers are mapped to physical ones and those have to be loaded as well.
 * r10 in translation mode points to the global arm_state and r11 has a copy of
 * cpsr_nzcv that is updated after every virtual instruction if it changes the
 * flags. After returning from translation mode because either a branch was
 * executed or the translated block ends, r11 is split into cpsr_nzcv again
 * and all registers restored.
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

// Uncomment the following line to measure the time until Boot2
// #define BENCHMARK
#ifdef BENCHMARK
    #include <ctime>
#endif

// Uncomment the following line to support relative jumps if possible,
// it doesn't work that often as the mmaped section is too far away.
// #define REL_BRANCH

#include "asmcode.h"
#include "cpu.h"
#include "cpudefs.h"
#include "mem.h"
#include "mmu.h"
#include "translate.h"
#include "os/os.h"

#ifdef __thumb__
#error Thumb mode is not supported!
#endif

extern "C" {
extern void translation_next() __asm__("translation_next");
extern void **translation_sp __asm__("translation_sp");
extern void *translation_pc_ptr __asm__("translation_pc_ptr");
#ifdef IS_IOS_BUILD
    int sys_cache_control(int function, void *start, size_t len);
#endif
}

#define MAX_TRANSLATIONS 0x40000
struct translation translation_table[MAX_TRANSLATIONS];
uint32_t *jump_table[MAX_TRANSLATIONS*2],
         **jump_table_current = jump_table,
         map_table[MAX_TRANSLATIONS*2],
         *map_table_current = map_table;

static uint32_t *translate_buffer = nullptr,
         *translate_current = nullptr,
         *translate_end = nullptr;

static unsigned int next_translation_index = 0;

static inline void emit(uint32_t instruction)
{
    *translate_current++ = instruction;
}

static inline void emit_al(uint32_t instruction)
{
    emit(instruction | (CC_AL << 28));
}

// Loads arm.reg[rs] into rd.
static void emit_ldr_armreg(const int rd, const int r_virt)
{
    emit_al(0x59a0000 | (rd << 12) | (r_virt << 2)); // ldr rd, [r10, #2*r_virt]
}

// Saves rd into arm.reg[rs].
static void emit_str_armreg(const int rd, const int r_virt)
{
    emit_al(0x58a0000 | (rd << 12) | (r_virt << 2)); // str rd, [r10, #2*r_virt]
}

// Register allocation: Registers are dynamically reused
enum Reg : uint8_t {
    R0=0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,SP,LR,PC,
    Invalid=16,Dirty=128
};

static const int first_map_reg = R4,
                    last_map_reg = R9;

// Maps virtual register to physical register (or invalid) and a dirty-flag
// PC is never mapped
static uint8_t regmap[15];

// Maps physical register to virtual register, mustn't contain anything > 0xF
static uint8_t mapreg[15];

// The next available physical register
static unsigned int current_reg = first_map_reg;

// Physical registers that must not be unmapped
static const int MAX_USED_REGS = 4;
static uint8_t used_regs[MAX_USED_REGS];

// How many regs are currently used and must not be unmapped
static unsigned int current_used_regs_count = 0;

static void map_reset()
{
    std::fill(regmap, regmap + 15, Invalid);
    current_used_regs_count = 0;
    current_reg = first_map_reg;
    // mapreg doesn't have to be reset and mustn't contain anything > 0xF anyway
    //std::fill(mapreg, mapreg + 15, Invalid);
}

// Marks a reg as needed. It won't be unmapped until the next instruction
static int map_need(int reg)
{
    assert(current_used_regs_count < MAX_USED_REGS);
    used_regs[current_used_regs_count++] = reg;
    return reg;
}

// Whether the physical register reg is needed
static bool reg_used(int reg)
{
    if(current_used_regs_count == 0)
        return false;

    for(unsigned int i = 0; i < current_used_regs_count; ++i)
        if(used_regs[i] == reg)
            return true;

    return false;
}

// Updates current_reg to point to an unneeded one
static void next_reg()
{
    do
    {
        if(current_reg == last_map_reg)
            current_reg = first_map_reg;
        else
            ++current_reg;
    }
    while(reg_used(current_reg));
}

// Flushed reg_phys into arm if needed and invalidates the map entriy
static void unmap_reg(int reg_phys)
{
    int oldvirt = mapreg[reg_phys];
    // Mapped?
    if((regmap[oldvirt] & 0xF) != current_reg)
        return;

    // Save register value
    if(regmap[oldvirt] & Dirty)
        emit_str_armreg(current_reg, oldvirt);

    // Invalidate map entry
    regmap[oldvirt] = Invalid;
}

static int map_reg(int reg_virt, bool write)
{
    assert(reg_virt <= 14); // Must not happen: PC (15) not implemented and any higher nr is invalid

    // Already mapped?
    if(regmap[reg_virt] != Invalid)
    {
        if(write)
            regmap[reg_virt] |= Dirty;
        return regmap[reg_virt] & 0xF; // Mask out the dirty bit
    }

    // Adjust current_reg to point to an unused one
    next_reg();

    // Reg not loaded, reuse register: Remove old mapping first
    unmap_reg(current_reg);

    // Set map entry and load value
    mapreg[current_reg] = reg_virt;
    if(write)
        regmap[reg_virt] = current_reg | Dirty;
    else
    {
        // Load reg from memory
        emit_ldr_armreg(current_reg, reg_virt);
        regmap[reg_virt] = current_reg;
    }

    return current_reg;
}

// The reg is marked as dirty
// Return value: Register number the reg has been mapped to
static int map_save_reg(int reg)
{
    int ret = map_reg(reg, true);
    return map_need(ret);
}

// The current value is loaded, if not mapped yet
// Return value: Register number the reg has been loaded into
static int map_load_reg(int reg)
{
    int ret = map_reg(reg, false);
    return map_need(ret);
}

// Load and store flags from r11
static void emit_ldr_flags()
{
    emit_al(0x128f00b); // msr cpsr_f, r11
}

static void emit_str_flags()
{
    emit_al(0x10fb000); // mrs r11, cpsr_f
    // r11 is written to arm.cpsr_flags in translation_next and the memory access helpers in asmcode_arm.S
}

// Sets phys. rd to phys. rn
static void emit_mov_reg(int rd, int rn)
{
    emit_al(0x1a00000 | (rd << 12) | rn);
}

// Sets phys. rd to imm
static void emit_mov_imm(int rd, uint32_t imm)
{
    // movw/movt only available on >= armv7
    #if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__)
        emit_al(0x3000000 | (rd << 12) | ((imm & 0xF000) << 4) | (imm & 0xFFF)); // movw rd, #imm&0xFFFF
        imm >>= 16;
        if(imm)
            emit_al(0x3400000 | (rd << 12) | ((imm & 0xF000) << 4) | (imm & 0xFFF)); // movt rd, #imm>>16
    #else
        if((imm & 0xFF) == imm)
            emit_al(0x3a00000 | (rd << 12) | imm); // mov rd, #imm
        else
        {
            // Insert immediate into code and jump over it
            emit_al(0x59f0000 | (rd << 12)); // ldr rd, [pc]
            emit_al(0xa000000); // b pc
            emit(imm);
        }
    #endif
}

// Returns 0 if target not reachable or a branch instruction
static uint32_t maybe_branch(void *target)
{
    #ifdef REL_BRANCH
        // The difference is a count of 4-bytes already, don't shift
        ptrdiff_t diff = reinterpret_cast<uint32_t*>(target) - translate_current - 2;
        if(diff > 0x7FFFFF || -diff > 0x800000)
            return 0;

        return 0xa000000 | (diff & 0xFFFFFF);
    #else
        (void) target;
        return 0; // Never use rel. branches. Tests below are optimized out.
    #endif
}

static void emit_jmp(void *target)
{
    uint32_t branch = maybe_branch(target);
    if(branch)
        return emit_al(branch);

    // Doesn't fit into branch, use memory
    emit_al(0x51ff004); // ldr pc, [pc, #-4]
    emit(reinterpret_cast<uintptr_t>(target));
}

// Flush all mapped regs back into arm.reg[]
static void emit_flush_regs()
{
    for(unsigned int i = 0; i < 15; ++i)
    {
        if(regmap[i] & Dirty)
            emit_str_armreg(regmap[i], i);

        // mapreg doesn't have to be reset and mustn't contain anything > 0xF anyway
        // mapreg[regmap[i] & 0xF] = Invalid;
        regmap[i] = Invalid;
    }
}

// Flush all regs into the global arm_state struct
// Has to be called before anything that might cause a translation leave!
static void emit_save_state()
{
    emit_flush_regs();
}

// Registers r0-r3 and r12 are not preserved!
static void emit_call(void *target, bool save = true)
{
    if(save)
        emit_save_state();

    uint32_t branch = maybe_branch(target);
    if(branch)
        return emit_al(branch | (1 << 24)); // Set the L-bit

    // This is cheaper than doing it like emit_mov above.
    emit_al(0x28fe004); // add lr, pc, #4
    emit_al(0x51ff004); // ldr pc, [pc, #-4]
    emit(reinterpret_cast<uintptr_t>(target));
}

static __attribute__((unused)) void emit_bkpt()
{
    emit_al(0x1200070); // bkpt #0
}

bool translate_init()
{
    if(translate_buffer)
        return true;

    map_reset();
    translate_current = translate_buffer = reinterpret_cast<uint32_t*>(os_alloc_executable(INSN_BUFFER_SIZE));
    translate_end = translate_current + INSN_BUFFER_SIZE/sizeof(*translate_buffer);
    jump_table_current = jump_table;
    map_table_current = map_table;
    next_translation_index = 0;

    return !!translate_buffer;
}

void translate_deinit()
{
    if(!translate_buffer)
        return;

    os_free(translate_buffer, INSN_BUFFER_SIZE);
    translate_end = translate_current = translate_buffer = nullptr;
}

void translate(uint32_t pc_start, uint32_t *insn_ptr_start)
{
    if(next_translation_index >= MAX_TRANSLATIONS)
    {
        warn("Out of translation slots!");
        return;
    }

    uint32_t **jump_table_start = jump_table_current, *map_table_start = map_table_current;
    uint32_t pc = pc_start, *insn_ptr = insn_ptr_start;
    // Pointer to translation of current instruction
    uint32_t *translate_buffer_inst_start = translate_current;
    // cond_branch points to the bne, see below
    uint32_t *cond_branch = nullptr;
    // Pointer to struct translation for this block
    translation *this_translation = &translation_table[next_translation_index];

    // We know this already. end_ptr will be set after the loop
    this_translation->jump_table = reinterpret_cast<void**>(jump_table_start);
    this_translation->map_table = reinterpret_cast<void**>(map_table_start);
    this_translation->start_ptr = insn_ptr_start;

    #ifdef BENCHMARK
        static clock_t start = 0;
        if(pc == 0)
            start = clock();
        else if(pc == 0x11800000)
        {
            clock_t diff = clock() - start;
            printf("%ld ms\n", diff / 1000);
        }
    #endif

    // This loop is executed once per instruction
    // Due to the CPU being able to jump to each instruction seperately,
    // there is no state preserved, so register mappings are reset.
    while(1)
    {
        // Translate further?
        if(translate_current + 0x200 > translate_end
            || RAM_FLAGS(insn_ptr) & (RF_EXEC_BREAKPOINT | RF_EXEC_DEBUG_NEXT | RF_EXEC_HACK | RF_CODE_TRANSLATED | RF_CODE_NO_TRANSLATE)
            || (pc ^ pc_start) & ~0xfff)
            goto exit_translation;

        Instruction i;
        uint32_t insn = i.raw = *insn_ptr;

        // It may be that some reg mappings are flushed into memory,
        // but skipped. So flush everything now.
        // As translate_buffer_inst_start is set after this, emit_save_state counts as the last instruction
        if(i.cond != CC_AL)
            emit_save_state();

        // Right now no regs are needed
        current_used_regs_count = 0;

        // Rollback translate_current to this val if instruction not supported
        *jump_table_current = translate_buffer_inst_start = translate_current;

        // A map_table entry is in the format 0x87654321,
        // indicating that r4 is arm.reg[1], r5 arm.reg[2] and so on.
        // Should first_map_reg or last_map_reg change, update this and asmcode_arm.S.
        static_assert(first_map_reg == R4 && last_map_reg == R9, "Update the code below!");
        *map_table_current = (mapreg[R9] << 20)
                           | (mapreg[R8] << 16)
                           | (mapreg[R7] << 12)
                           | (mapreg[R6] << 8)
                           | (mapreg[R5] << 4)
                           | mapreg[R4];

        //printf("At 0x%x: %x %x %x %x %x %x\n", pc, mapreg[R4], mapreg[R5], mapreg[R6], mapreg[R7], mapreg[R8], mapreg[R9]);
        //printf("0x%x %p\n", pc, translate_current);

        // Conditional instructions are translated like this:
        // msr cpsr_f, r11
        // b<cc> after_inst
        // <instruction>
        // after_inst: @ next instruction
        if(i.cond != CC_AL)
        {
            emit_ldr_flags();
            cond_branch = translate_current;
            emit(0x0a000000 | ((i.cond ^ 1) << 28));
        }

        if((insn & 0xE000090) == 0x0000090)
            goto unimpl;
        else if((insn & 0xD900000) == 0x1000000)
        {
            // Only msr rd, cpsr is supported
            if((insn & 0xFF00000) != 0x10F0000)
                goto unimpl;

            // Load cpsr (in phys. r11) into register (virt. rd)
            emit_mov_reg(map_save_reg(i.mrs.rd), R11);
        }
        else if((insn & 0xC000000) == 0x0000000)
        {
            // Data processing:
            // The registers the instruction uses are renamed
            bool setcc = i.data_proc.s,
                 reg_shift = !i.data_proc.imm && i.data_proc.reg_shift;

            // Using pc as src reg is not supported
            if(i.data_proc.rn == PC // First operand
                    || (!i.data_proc.imm && i.data_proc.rm == PC) // Second operand
                    || (reg_shift && i.data_proc.rs == PC) // Register shift
                    || (setcc && i.data_proc.rd == PC)) // Exception return
                goto unimpl;

			Instruction translated;
			translated.raw = i.raw;

			// Map needed register values:
			// add rd, rn, rm, lsl rs becomes add r4, r0, r1, lsl r2 for example

			// MOV and MVN don't have Rn
			if(i.data_proc.op != OP_MOV && i.data_proc.op != OP_MVN)
                translated.data_proc.rn = map_load_reg(i.data_proc.rn);

			// rm is stored in the immediate, don't overwrite it
			if(!i.data_proc.imm)
                translated.data_proc.rm = map_load_reg(i.data_proc.rm);

			// Only load rs if actually needed
            if(reg_shift)
                translated.data_proc.rs = map_load_reg(i.data_proc.rs);

			// Only change the destination register if it has one
            if(i.data_proc.op < OP_TST || i.data_proc.op > OP_CMN)
            {
                if(i.data_proc.rd != PC)
                    translated.data_proc.rd = map_save_reg(i.data_proc.rd);
                else
                    translated.data_proc.rd = R0; // R0 is first arg to translation_next
            }

            if(unlikely(setcc
                        || i.data_proc.op == OP_ADC
                        || i.data_proc.op == OP_SBC
                        || i.data_proc.op == OP_RSC
                        || (!i.data_proc.reg_shift && i.data_proc.shift == SH_ROR))) // ROR with #0 as imm is RRX (not everything checked)
            {
                // This instruction impacts the flags, load them...
                emit_ldr_flags();

                // Emit the instruction itself
                emit(translated.raw);

                // ... and store them again
                emit_str_flags();
            }
            else
                emit(translated.raw);

            // Jump to destinatino
            if(i.data_proc.rd == PC)
            {
                // We're going to leave the translation, flush regs
                emit_save_state();
                emit_jmp(reinterpret_cast<void*>(translation_next));
            }
        }
        else if((insn & 0xC000000) == 0x4000000)
        {goto unimpl;
            // Memory access: LDR, STRB, etc.

            // User mode access not implemented
            if(!i.mem_proc.p && i.mem_proc.w)
                goto unimpl;

            // pc-relative not implemented
            if(i.mem_proc.rn == 15 || i.mem_proc.rm == 15)
                goto unimpl;

            // Base register gets in r0
            emit_ldr_armreg(0, i.mem_proc.rn);

            bool offset_is_zero = !i.mem_proc.not_imm && i.mem_proc.immed == 0;

            // Skip offset calculation
            if(offset_is_zero)
                goto no_offset;

            // Offset gets in r5
            if(!i.mem_proc.not_imm)
            {
                // Immediate offset
                emit_mov_imm(5, i.mem_proc.immed);
            }
            else
            {
                // Shifted register: translate to mov
                Instruction off;
                off.raw = 0xe1a05000; // mov r5, something
                off.raw |= insn & 0xFFF; // Copy shifter_operand
                off.data_proc.rm = 1;
                emit_ldr_armreg(1, i.mem_proc.rm);
                emit(off.raw);
            }

            // Get final address in r5
            if(i.mem_proc.u)
                emit_al(0x0805005); // add r5, r0, r5
            else
                emit_al(0x0405005); // sub r5, r0, r5

            // Get final address in r0
            if(i.mem_proc.p)
            {
                emit_al(0x1a00005); // mov r0, r5
                if(i.mem_proc.w) // Writeback: final address into rn
                    emit_str_armreg(0, i.mem_proc.rn);
            }

            no_offset:
            if(i.mem_proc.l)
            {
                emit_call(reinterpret_cast<void*>(i.mem_proc.b ? read_byte_asm : read_word_asm));
                if(i.mem_proc.rd != 15)
                    emit_str_armreg(0, i.mem_proc.rd); // r0 is return value
                else
                {
                    //TODO: Fix this
                    //goto unimpl;
                    // pc is destination register
                    emit_save_state();
                    emit_jmp(reinterpret_cast<void*>(translation_next));
                }
            }
            else
            {
                emit_ldr_armreg(1, i.mem_proc.rd); // r1 is the value
                emit_call(reinterpret_cast<void*>(i.mem_proc.b ? write_byte_asm : write_word_asm));
            }

            // Post-indexed: final address in r5 back into rn
            if(!offset_is_zero && !i.mem_proc.p)
                emit_str_armreg(5, i.mem_proc.rn);
        }
        else if((insn & 0xE000000) == 0xA000000)
        {goto unimpl;
            /* Branches work this way:
             * Either jump to translation_next if code not translated (yet) or
             * jump directly to the translated code, over a small function checking for pending events */

            if(i.branch.l)
            {
                // Save return address in LR
                emit_mov_imm(R0, pc + 4);
                emit_str_armreg(R0, LR);
            }

            // We're going to jump somewhere else
            emit_save_state();

            uint32_t addr = pc + ((int32_t) i.raw << 8 >> 6) + 8;
            uintptr_t entry = reinterpret_cast<uintptr_t>(addr_cache[(addr >> 10) << 1]);
            uint32_t *ptr = reinterpret_cast<uint32_t*>(entry + addr);
            if((entry & AC_FLAGS) || !(RAM_FLAGS(ptr) & RF_CODE_TRANSLATED))
            {
                // Not translated, use translation_next
                emit_mov_imm(R0, addr);
                emit_jmp(reinterpret_cast<void*>(translation_next));
            }
            else
            {
                // Get address of translated code to jump to it
                translation *target_translation = &translation_table[RAM_FLAGS(ptr) >> RFS_TRANSLATION_INDEX];
                uintptr_t jmp_target = reinterpret_cast<uintptr_t>(target_translation->jump_table[ptr - target_translation->start_ptr]);

                // Update pc first
                emit_mov_imm(R0, addr);
                emit_str_armreg(R0, PC);
                emit_mov_imm(R0, jmp_target);
                emit_jmp(reinterpret_cast<void*>(translation_jmp));
            }
        }
        else
            goto unimpl;

        if(cond_branch)
        {
            // The mappings that happened during the execution may have been skipped
            // so they can't be used.
            emit_save_state();

            // Fixup the branch above (-2 to account for the pipeline)
            *cond_branch |= (translate_current - cond_branch - 2) & 0xFFFFFF;
            cond_branch = nullptr;
        }

        RAM_FLAGS(insn_ptr) |= (RF_CODE_TRANSLATED | next_translation_index << RFS_TRANSLATION_INDEX);
        ++jump_table_current;
        ++map_table_current;
        ++insn_ptr;
        pc += 4;
    }

    unimpl:
    // There may be a partial translation in memory, scrap it.
    translate_current = translate_buffer_inst_start;
    RAM_FLAGS(insn_ptr) |= RF_CODE_NO_TRANSLATE;

	exit_translation:
	emit_save_state();
    emit_mov_imm(0, pc);
    emit_jmp(reinterpret_cast<void*>(translation_next));

    // Did we do any translation at all?
    if(insn_ptr == insn_ptr_start)
        return;

    this_translation->end_ptr = insn_ptr;
    // This effectively flushes this_translation, as it won't get used next time
    next_translation_index += 1;

    // Flush the instruction cache
#ifdef IS_IOS_BUILD
    sys_cache_control(1 /* kCacheFunctionPrepareForExecution */, jump_table_start[0], (translate_current-jump_table_start[0])*4);
#else
    __builtin___clear_cache(jump_table_start[0], translate_current);
#endif
}

void flush_translations()
{
    for(unsigned int index = 0; index < next_translation_index; index++)
    {
        uint32_t *start = translation_table[index].start_ptr;
        uint32_t *end   = translation_table[index].end_ptr;
        for (; start < end; start++)
            RAM_FLAGS(start) &= ~(RF_CODE_TRANSLATED | (-1 << RFS_TRANSLATION_INDEX));
    }

    next_translation_index = 0;
    translate_current = translate_buffer;
    jump_table_current = jump_table;
}

void invalidate_translation(int index)
{
    if(!translation_sp)
        return flush_translations();

    uint32_t flags = RAM_FLAGS(translation_pc_ptr);
    if ((flags & RF_CODE_TRANSLATED) && (int)(flags >> RFS_TRANSLATION_INDEX) == index)
        error("Cannot modify currently executing code block.");

    flush_translations();
}

void translate_fix_pc()
{
    if (!translation_sp)
        return;

    // This is normally done when leaving the translation,
    // but since we are here, this didn't happen (longjmp)
    set_cpsr_flags(arm.cpsr_flags);

    // Not implemented: Get accurate pc back in arm.reg[15]
    assert(false);
}
