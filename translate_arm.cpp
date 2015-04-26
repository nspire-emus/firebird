/*
 * How the ARM translation works:
 * translation_enter in asmcode_arm.S finds the entry point and jumps to it.
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
         **jump_table_current = jump_table;

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
static void emit_ldr_armreg(uint8_t rd, uint8_t r_virt)
{
    emit_al(0x59a0000 | (rd << 12) | (r_virt << 2)); // ldr rd, [r10, #2*r_virt]
}

// Saves rd into arm.reg[rs].
static void emit_str_armreg(uint8_t rd, uint8_t r_virt)
{
    emit_al(0x58a0000 | (rd << 12) | (r_virt << 2)); // str rd, [r10, #2*r_virt]
}

// Register allocation: Registers are dynamically reused
enum Reg : uint8_t {
	R0=0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,SP,LR,PC,
    Invalid=16,Dirty=128
};

// Maps virtual register to physical register (or invalid) and a dirty-flag
// PC is never mapped
static uint8_t regmap[15];
#define REG(x) (x&0xF)

// The next available physical register
static uint8_t current_reg = R4;

static void map_reset()
{
    std::fill(regmap, regmap + 15, Invalid);
}

static int map_reg(uint8_t reg, bool write)
{
    assert(reg < 16); // Must not happen: PC not implemented and higher nr is invalid

    // Already mapped?
    if(regmap[reg] != Invalid)
    {
        if(write)
            regmap[reg] |= Dirty;
        return REG(regmap[reg]);
    }

    // Reg not loaded, reuse register: Remove old mapping first
    for(unsigned int i = 0; i < 15; ++i)
    {
        if(REG(regmap[i]) != current_reg)
            continue;

        // Save register value
        if(regmap[i] & Dirty)
            emit_str_armreg(current_reg, i);

        regmap[i] = Invalid;
        break; // Always only one mapping present
    }

    if(write)
        regmap[reg] = current_reg | Dirty;
    else
    {
        // Load reg from memory
        emit_ldr_armreg(current_reg, reg);
        regmap[reg] = current_reg;
    }

    uint8_t ret = current_reg++;
    // Wrap to R4 after all regs cycled
    if(current_reg == R10)
        current_reg = R4;

    return ret;
}

// After eight loaded registers the previous ones will be reused!
// The reg is marked as dirty
// Return value: Register number the reg has been mapped to
static int map_save_reg(uint8_t reg)
{
    return map_reg(reg, true);
}

// After eight loaded registers the previous ones will be reused!
// The current value is loaded, if not mapped yet
// Return value: Register number the reg has been loaded into
static int map_load_reg(uint8_t reg)
{
    return map_reg(reg, false);
}

// Flush all mapped regs back into arm.reg[]
static void emit_flush_regs()
{
	for(unsigned int i = 0; i < 15; ++i)
    {
        if(regmap[i] & Dirty)
            emit_str_armreg(regmap[i], i);
        regmap[i] = Invalid;
    }
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

// Sets rd to imm
static void emit_mov(uint8_t rd, uint32_t imm)
{
	if((imm & 0xFF) == imm)
        emit_al(0x3a00000 | (rd << 12) | imm); // mov rd, #imm
	else
    {
        // movw/movt only available on >= armv7
        #ifdef __ARM_ARCH_7__
                emit_al(0x3000000 | (rd << 12) | ((imm & 0xF000) << 4) | (imm & 0xFFF)); // movw rd, #imm&0xFFFF
                imm >>= 16;
                if(imm)
                    emit_al(0x3400000 | (rd << 12) | ((imm & 0xF000) << 4) | (imm & 0xFFF)); // movt rd, #imm>>16
        #else
                // Insert immediate into code and jump over it
                emit_al(0x59f0000 | (rd << 12)); // ldr rd, [pc]
                emit_al(0xa000000); // b pc
                emit(imm);
        #endif
    }
}

static void emit_jmp(void *target)
{
    emit_al(0x51ff004); // ldr pc, [pc, #-4]
    emit(reinterpret_cast<uintptr_t>(target));
}

static void emit_save_state();

// Registers r0-r3 and r12 are not preserved!
static void emit_call(void *target, bool save = true)
{
	if(save)
		emit_save_state();

    // This is cheaper than doing it like emit_mov above.
    emit_al(0x28fe004); // add lr, pc, #4
    emit_jmp(target);
}

// Flush all regs into the global arm_state struct
// Has to be called before anything that might cause a translation leave!
static void emit_save_state()
{
	emit_flush_regs();
}

static __attribute__((unused)) void emit_bkpt()
{
    emit_al(0x1200070); // bkpt #0
}

bool translate_init()
{
    if(translate_buffer)
        return true;

    translate_current = translate_buffer = reinterpret_cast<uint32_t*>(os_alloc_executable(INSN_BUFFER_SIZE));
    translate_end = translate_current + INSN_BUFFER_SIZE/sizeof(*translate_buffer);
    jump_table_current = jump_table;
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

    map_reset();

    uint32_t **jump_table_start = jump_table_current;
    uint32_t pc = pc_start, *insn_ptr = insn_ptr_start;
    // Pointer to translation of current instruction
    uint32_t *translate_buffer_inst_start = translate_current;
    // cond_branch points to the bne, see below
    uint32_t *cond_branch = nullptr;
    // Pointer to struct translation for this block
    translation *this_translation = &translation_table[next_translation_index];

    // We know this already. end_ptr will be set after the loop
    this_translation->jump_table = reinterpret_cast<void**>(jump_table_start);
    this_translation->start_ptr = insn_ptr_start;

    // This loop is executed once per instruction
    // Due to the CPU being able to jump to each instruction seperately,
    // there is no state preserved, so register mappings are reset.
    while(1)
    {
        // Translate further?
        if(translate_current + 0x200 > translate_end
            || RAM_FLAGS(insn_ptr) & (RF_EXEC_BREAKPOINT | RF_EXEC_DEBUG_NEXT | RF_EXEC_HACK | RF_CODE_TRANSLATED | RF_CODE_NO_TRANSLATE)
            || (pc ^ pc_start) & ~0x3ff)
            goto exit_translation;

        Instruction i;
        uint32_t insn = i.raw = *insn_ptr;

        // Rollback translate_current to this val if instruction not supported
        translate_buffer_inst_start = translate_current;

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

            // Store cpsr in arm.reg[rd]
            emit_str_armreg(11, i.mrs.rd);
        }
        else if((insn & 0xC000000) == 0x0000000)
        {
            // Data processing:
            // The registers the instruction uses are renamed
            bool setcc = i.data_proc.s,
                 reg_shift = !i.data_proc.imm && i.data_proc.reg_shift;

            // Using pc is not supported
            if(i.data_proc.rd == 15
                    || i.data_proc.rn == 15
                    || i.data_proc.rm == 15
                    || (reg_shift && i.data_proc.rs == 15))
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
                translated.data_proc.rd = map_save_reg(i.data_proc.rd);

            if(unlikely(setcc
                        || i.data_proc.op == OP_ADC
                        || i.data_proc.op == OP_SBC
                        || i.data_proc.op == OP_RSC
                        || i.data_proc.shift == SH_ROR)) // ROR with #0 as imm is RRX
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
        }
        else if((insn & 0xC000000) == 0x4000000)
        {
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
                emit_mov(5, i.mem_proc.immed);
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
                    goto unimpl;
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
        {
            /* Branches work this way:
             * Either jump to translation_next if code not translated (yet) or
             * jump directly to the translated code, over a small function checking for pending events */

            if(i.branch.l)
            {
                // Save return address in LR
                emit_mov(R0, pc + 4);
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
                emit_mov(R0, addr);
                emit_jmp(reinterpret_cast<void*>(translation_next));
            }
            else
            {
                // Get address of translated code to jump to it
                translation *target_translation = &translation_table[RAM_FLAGS(ptr) >> RFS_TRANSLATION_INDEX];
                uintptr_t jmp_target = reinterpret_cast<uintptr_t>(target_translation->jump_table[ptr - target_translation->start_ptr]);

                // Update pc first
                emit_mov(R0, addr);
                emit_str_armreg(R0, PC);
                emit_mov(R0, jmp_target);
                emit_jmp(reinterpret_cast<void*>(translation_jmp));
            }
        }
        else
            goto unimpl;

        emit_save_state();

        if(cond_branch)
        {
            // Fixup the branch above (-2 to account for the pipeline)
            *cond_branch |= (translate_current - cond_branch - 2) & 0xFFFFFF;
            cond_branch = nullptr;
        }

        RAM_FLAGS(insn_ptr) |= (RF_CODE_TRANSLATED | next_translation_index << RFS_TRANSLATION_INDEX);
        *jump_table_current++ = translate_buffer_inst_start;
        ++insn_ptr;
        pc += 4;
    }

    unimpl:
    // There may be a partial translation in memory, scrap it.
    translate_current = translate_buffer_inst_start;
    RAM_FLAGS(insn_ptr) |= RF_CODE_NO_TRANSLATE;

	exit_translation:
	emit_save_state();
    emit_mov(0, pc);
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
