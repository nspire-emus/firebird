/*
 * How the ARM translation works:
 * translation_enter in asmcode_arm.S finds the entry point and jumps to it.
 * r10 in translation mode points to the global arm_state and the cpsr contains
 * the flags of the virtual processor. To preserve them across function calls
 * and to save them back into struct arm_state, r11 is used.  After returning
 * from translation mode because either a branch was executed or the translated
 * block ends, r11 is split into cpsr_nzcv again and all registers restored.
 */

#include <cassert>
#include <cstdint>
#include <cstdio>

#ifdef IS_IOS_BUILD
#include <sys/syscall.h>
#include <sys/mman.h>
#endif

// Uncomment the following line to support relative jumps if possible,
// it doesn't work that often as the mmaped section is too far away.
// #define REL_BRANCH

#include "asmcode.h"
#include "cpu.h"
#include "cpudefs.h"
#include "disasm.h"
#include "mem.h"
#include "mmu.h"
#include "translate.h"
#include "os/os.h"

#ifdef __thumb__
#error Thumb mode is not supported!
#endif

extern "C" {
extern void translation_next() __asm__("translation_next");
extern void translation_next_bx() __asm__("translation_next_bx");
extern void **translation_sp __asm__("translation_sp");
#ifdef IS_IOS_BUILD
    int sys_cache_control(int function, void *start, size_t len);
#endif
}

#define MAX_TRANSLATIONS 0x40000
struct translation translation_table[MAX_TRANSLATIONS];
uint32_t *jump_table[MAX_TRANSLATIONS*2],
         **jump_table_current = jump_table;

uint32_t *translate_buffer = nullptr,
         *translate_current = nullptr,
         *translate_end = nullptr;

#include "literalpool.h"

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
static void emit_ldr_armreg(const unsigned int rd, const unsigned int r_virt)
{
    assert(rd < 16 && r_virt < 16);
    emit_al(0x59a0000 | (rd << 12) | (r_virt << 2)); // ldr rd, [r10, #2*r_virt]
}

// Saves rd into arm.reg[rs].
static void emit_str_armreg(const unsigned int rd, const unsigned int r_virt)
{
    assert(rd < 16 && r_virt < 16);
    emit_al(0x58a0000 | (rd << 12) | (r_virt << 2)); // str rd, [r10, #2*r_virt]
}

// Load and store flags from r11
static void emit_ldr_flags()
{
    emit_al(0x128f00b); // msr cpsr_f, r11
}

static __attribute__((unused)) void emit_bkpt()
{
    emit_al(0x1200070); // bkpt #0
}

static void emit_str_flags()
{
    emit_al(0x10fb000); // mrs r11, cpsr_f
    // r11 is written to arm.cpsr_flags in translation_next and the memory access helpers in asmcode_arm.S
}

// Sets rd to imm
static void emit_mov(int rd, uint32_t imm)
{
    // movw/movt only available on >= armv7
    #if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__)
        if(imm <= 0xFFFF)
        {
            emit_al(0x3000000 | (rd << 12) | ((imm & 0xF000) << 4) | (imm & 0xFFF)); // movw rd, #imm&0xFFFF
            imm >>= 16;
            if(imm)
                emit_al(0x3400000 | (rd << 12) | ((imm & 0xF000) << 4) | (imm & 0xFFF)); // movt rd, #imm>>16

            return;
        }
    #else
        if(imm <= 0xFF)
        {
            emit_al(0x3a00000 | (rd << 12) | imm); // mov rd, #imm
            return;
        }
    #endif

    literalpool_add(imm);
    emit_al(0x51f0000 | (rd << 12)); // ldr rd, [pc, #<offset>]
}

static void emit_mov_reg(uint8_t rd, uint8_t rm)
{
    emit_al(0x1a00000 | (rd << 12) | rm);
}

void literalpool_fill()
{
    for(unsigned int i = 0; i < literals_count; ++i)
    {
        auto &&literal = literals[i];
        if(!literal.inst)
            continue;

        // Emit the literal
        uint32_t *literal_loc = translate_current;
        emit(static_cast<uint32_t>(literal.value));

        // Fixup all literal references for the same value
        for(unsigned int j = i; j < literals_count; ++j)
        {
            auto &&literal_ref = literals[j];
            if(!literal_ref.inst || literal_ref.value != literal.value)
                continue;

            ptrdiff_t diff = reinterpret_cast<uint8_t*>(literal_loc) - reinterpret_cast<uint8_t*>(literal_ref.inst) - 8;

            if(diff >= 0)
                *reinterpret_cast<uint32_t*>(literal_ref.inst) |= 1 << 23; // Set U-bit
            else
                diff = -diff;

            if(diff > 0xFFF)
                error("Literal unreachable!");

            *reinterpret_cast<uint32_t*>(literal_ref.inst) |= diff;
            literal_ref.inst = nullptr;
        }
    }

    literals_count = 0;
}

enum Reg : uint8_t {
    R0 = 0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, SP, LR, PC
};

/* Register mapping.
   To translate a set of instructions (a basic block), virtual registers are loaded/mapped
   to physical registers with certain flags. */

/* Maps virtual registers r0-lr to physical registers. 0 means not mapped. X means register rX-1 and -X means register r1-X (dirty). */
static int8_t regmap_v2p[16];

/* R0 is used as scratch register. R10 is a pointer to struct arm_state, R11 contains flags. */
static constexpr const uint8_t regmap_first_phys = R1, regmap_last_phys = R9;

/* Shortcut. If anything has been mapped, this is true.
   Set by regmap_next_preg, unset by regmap_flush. */
static bool regmap_any_mapped = false;

/* Shortcut. The next available phys reg for mapping. */
static uint8_t regmap_next_phys = regmap_first_phys;

enum PhysRegFlags {
    UNMAPPED=0, /* Can be used for regmap_v2p as well */
    MAPPED=1,
    IN_USE=2
};
static PhysRegFlags regmap_pflags[16];

static void regmap_flush()
{
    if(!regmap_any_mapped)
        return;

    for(unsigned int i = 0; i < 16; ++i)
    {
        if(regmap_v2p[i] < 0)
            emit_str_armreg(-regmap_v2p[i] - 1, i);

        regmap_v2p[i] = UNMAPPED;
    }

    for(unsigned int i = regmap_first_phys; i <= regmap_last_phys; ++i)
        regmap_pflags[i] = UNMAPPED;

    regmap_any_mapped = false;
    regmap_next_phys = regmap_first_phys;
}

/* Returns the next free phyical register. It's marked as IN_USE. */
static uint8_t regmap_next_preg()
{
    regmap_any_mapped = true;

    // Try to find unused phys reg first
    for(unsigned int i = regmap_next_phys; i <= regmap_last_phys; ++i)
    {
        if(regmap_pflags[i] == UNMAPPED)
        {
            regmap_pflags[i] = IN_USE;
            regmap_next_phys = i + 1;
            return i;
        }
    }

    // Haven't found one yet, so search for an unused mapped one and unmap it
    for(unsigned int i = regmap_first_phys; i <= regmap_last_phys; ++i)
    {
        if(regmap_pflags[i] == MAPPED)
        {
            // Flush old contents
            for(unsigned int j = 0; j < 16; ++j)
            {
                if(regmap_v2p[j] == int8_t(i + 1))
                {
                    // Not dirty
                    regmap_v2p[j] = UNMAPPED;
                    regmap_pflags[i] = IN_USE;
                    return i;
                }
                else if(regmap_v2p[j] == -int8_t(i + 1))
                {
                    // Write back
                    emit_str_armreg(i, j);

                    regmap_v2p[j] = UNMAPPED;
                    regmap_pflags[i] = IN_USE;
                    return i;
                }
            }

            assert(!"Inconsistent regmap!");
        }
    }

    assert(!"No physical registers left!");
    __builtin_unreachable();
}

static uint8_t regmap_loadstore(uint8_t rvirt)
{
    assert(rvirt < PC);

    uint8_t preg;

    if(regmap_v2p[rvirt] < 0)
    {
        preg = -regmap_v2p[rvirt] - 1;
        regmap_pflags[preg] = IN_USE;
        return preg;
    }
    else if(regmap_v2p[rvirt] > 0)
    {
        preg = regmap_v2p[rvirt] - 1;
        regmap_pflags[preg] = IN_USE;
        regmap_v2p[rvirt] = -(preg + 1);
        return preg;
    }

    // Not yet mapped
    preg = regmap_next_preg();
    emit_ldr_armreg(preg, rvirt);
    regmap_v2p[rvirt] = -(preg + 1);
    return preg;
}

static uint8_t regmap_load(uint8_t rvirt)
{
    assert(rvirt < PC);

    uint8_t preg;

    if(regmap_v2p[rvirt] < 0)
    {
        preg = -regmap_v2p[rvirt] - 1;
        regmap_pflags[preg] = IN_USE;
        return preg;
    }
    else if(regmap_v2p[rvirt] > 0)
    {
        preg = regmap_v2p[rvirt] - 1;
        regmap_pflags[preg] = IN_USE;
        return preg;
    }

    // Not yet mapped
    preg = regmap_next_preg();
    emit_ldr_armreg(preg, rvirt);
    regmap_v2p[rvirt] = preg + 1;
    return preg;
}

static uint8_t regmap_store(uint8_t rvirt)
{
    assert(rvirt < PC);

    uint8_t preg;

    if(regmap_v2p[rvirt] < 0)
    {
        preg = -regmap_v2p[rvirt] - 1;
        regmap_pflags[preg] = IN_USE;
        return preg;
    }
    else if(regmap_v2p[rvirt] > 0)
    {
        preg = regmap_v2p[rvirt] - 1;
        regmap_pflags[preg] = IN_USE;
        regmap_v2p[rvirt] = -(preg + 1);
        return preg;
    }

    // Not yet mapped
    preg = regmap_next_preg();
    regmap_v2p[rvirt] = -(preg + 1);
    return preg;
}

static void regmap_mark_all_unused()
{
    for(unsigned int i = 0; i < 16; ++i)
    {
        if(regmap_pflags[i] == IN_USE)
             regmap_pflags[i] = MAPPED;
    }
}

// Returns 0 if target not reachable
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

static void emit_jmp(void *target, bool save = true)
{
    if(save)
    {
        regmap_flush();

        // Store the flags to r11, the callee is responsible now
        emit_str_flags();
    }

    uint32_t branch = maybe_branch(target);
    if(branch)
        return emit_al(branch);

    // Doesn't fit into branch, use literal
    emit_mov(PC, reinterpret_cast<uintptr_t>(target));
}

// Registers r0-r3 and r12 are not preserved!
static void emit_call(void *target, bool save = true)
{
    if(save)
    {
        regmap_flush();
        // Preserve the flags across calls
        emit_str_flags();
    }

    uint32_t branch = maybe_branch(target);
    if(branch)
        return emit_al(branch | (1 << 24)); // Set the L-bit

    // This is cheaper than doing it like emit_mov above.
    emit_mov(R3, reinterpret_cast<uintptr_t>(target));
    emit_al(0x12fff33); // blx r3

    if(save)
        emit_ldr_flags();
}

bool translate_init()
{
    if(translate_buffer)
        return true;

#ifdef IS_IOS_BUILD
    if (!iOS_is_debugger_attached())
    {
        syscall(SYS_ptrace, 0 /*PTRACE_TRACEME*/, 0, 0, 0);
        if (!iOS_is_debugger_attached())
        {
            fprintf(stderr, "Was not able to force ptrace to allow JIT :(\n");
            return false;
        }
    }
#endif

    translate_current = translate_buffer = reinterpret_cast<uint32_t*>(os_alloc_executable(INSN_BUFFER_SIZE));
    translate_end = translate_current + INSN_BUFFER_SIZE/sizeof(*translate_buffer);
    jump_table_current = jump_table;
    next_translation_index = 0;

    if(!translate_buffer)
        return false;

    #ifndef NDEBUG
        regmap_flush();

        assert(!regmap_any_mapped);

        // Basic sanity check: Load and map a few regs.
        assert(regmap_load(regmap_first_phys) == regmap_first_phys);
        assert(regmap_store(regmap_first_phys) == regmap_first_phys);
        assert(regmap_loadstore(regmap_first_phys) == regmap_first_phys);
        assert(regmap_loadstore(regmap_first_phys+1) == regmap_first_phys+1);
        assert(regmap_store(regmap_first_phys+1) == regmap_first_phys+1);
        assert(regmap_store(regmap_first_phys) == regmap_first_phys);

        assert(regmap_any_mapped);

        // Force all regs to be mapped
        for(unsigned int i = regmap_first_phys; i <= regmap_last_phys; ++i)
            assert(regmap_load(i) == i);

        //assert(regmap_load(regmap_last_phys+1); // This will (and needs to) fail
        regmap_mark_all_unused();

        // Reused mapping
        assert(regmap_load(regmap_last_phys+1) == regmap_first_phys);
        // Old one still stays
        assert(regmap_loadstore(regmap_first_phys+1) == regmap_first_phys+1);

        regmap_flush();
        assert(!regmap_any_mapped);

        translate_current = translate_buffer;
    #endif

    return true;
}

void translate_deinit()
{
    if(!translate_buffer)
        return;

    os_free(translate_buffer, INSN_BUFFER_SIZE);
    translate_end = translate_current = translate_buffer = nullptr;
}

static __attribute__((unused)) void dump_translation(int index)
{
    auto &translation = translation_table[index];

    uint32_t pc = phys_mem_addr(translation.start_ptr);
    uint32_t **cur_jump_table = reinterpret_cast<uint32_t**>(translation.jump_table);
    uint32_t *translated_insn = reinterpret_cast<uint32_t*>(*cur_jump_table);

    puts("--------------------");

    for(; reinterpret_cast<uintptr_t>(translated_insn) < translation.unused; ++translated_insn)
    {
        printf("%.08x: ", pc);
        disasm_arm_insn2(reinterpret_cast<uint32_t>(translated_insn), translated_insn);

        if(translated_insn == cur_jump_table[1])
        {
            ++cur_jump_table;
            pc += 4;
        }
    }
}

void translate(uint32_t pc_start, uint32_t *insn_ptr_start)
{
    if(next_translation_index >= MAX_TRANSLATIONS)
    {
        gui_debug_printf("Out of translation slots!");
        flush_translations();
        return;
    }

    if(translate_current + 0x100 > translate_end)
    {
        gui_debug_printf("Out of translation space!");
        flush_translations();
        return;
    }

    #ifdef IS_IOS_BUILD
        // Mark translate_buffer as RW_
        mprotect(translate_buffer, INSN_BUFFER_SIZE, PROT_READ | PROT_WRITE);
    #endif

    uint32_t **jump_table_start = jump_table_current;
    uint32_t pc = pc_start, *insn_ptr = insn_ptr_start;
    // Pointer to translation of current instruction
    uint32_t *translate_buffer_inst_start = translate_current;
    // cond_branch points to the bne, see below
    uint32_t *cond_branch = nullptr;
    // Pointer to struct translation for this block
    translation *this_translation = &translation_table[next_translation_index];
    // Whether to stop translating code
    bool stop_here = false;
    // Whether the instruction is an unconditional jump away
    bool jumps_away = false;

    // We know this already. end_ptr will be set after the loop
    this_translation->jump_table = reinterpret_cast<void**>(jump_table_start);
    this_translation->start_ptr = insn_ptr_start;

    // Assert that the translated code does not assume any state
    assert(!regmap_any_mapped);

    // This loop is executed once per instruction.
    // Due to the CPU being able to jump to each instruction seperately,
    // there is no state preserved between (virtual) instructions.
    while(1)
    {
        // Translate further?
        if(stop_here
            || translate_current + 0x100 > translate_end
            || RAM_FLAGS(insn_ptr) & DONT_TRANSLATE
            || (pc ^ pc_start) & ~0x3ff)
            break;

        Instruction i;
        uint32_t insn = i.raw = *insn_ptr;

        regmap_mark_all_unused();

        if(i.cond != CC_AL && i.cond != CC_NV)
        {
            /* Conditional instructions are translated like this:
               <load flags>
               b<cc> after_inst
               <instruction>
               after_inst: @ next instruction

               This means that register mappings done (and undone!) in <instruction>
               may not be effective, so we need to flush those *before and after*. */

            regmap_flush();
        }

        // PC-relative loads only have a 12bit offset, which may not be enough to reach the literal pool.
        // In those cases, insert the literal pool into the code and jump over it.
        if(literals_count > 0 && translate_current - reinterpret_cast<uint32_t*>(literals[0].inst) > 0x300)
        {
            uint32_t *jump_over_literal = translate_current;
            emit(0xea000000); // b <after>
            literalpool_fill();
            *jump_over_literal |= (translate_current - jump_over_literal - 2) & 0xFFFFFF;
        }

        // Rollback translate_current to this val if instruction not supported
        *jump_table_current = translate_buffer_inst_start = translate_current;

        bool can_jump_here = !regmap_any_mapped;

        if(unlikely(i.cond == CC_NV))
        {
            if((i.raw & 0xFD70F000) == 0xF550F000)
                goto instruction_translated; // PLD
            else
                goto unimpl;
        }
        else if(unlikely(i.cond != CC_AL))
        {
            cond_branch = translate_current;
            emit(0x0a000000 | ((i.cond ^ 1) << 28));
        }

        if((insn & 0xE000090) == 0x0000090)
        {
            if(!i.mem_proc2.s && !i.mem_proc2.h)
            {
                // Not mem_proc2 -> multiply or swap
                if((insn & 0x0FB00000) == 0x01000000)
                    goto unimpl; // SWP/SWPB not implemented

                // MUL, UMLAL, etc.
                if(i.mult.rm == PC || i.mult.rs == PC
                        || i.mult.rn == PC || i.mult.rd == PC)
                    goto unimpl; // PC as register not implemented

                // Register renaming, see data processing below
                Instruction translated;

                translated.raw = (i.raw & 0xFFFFFFF) | 0xE0000000;

                translated.mult.rm = regmap_load(i.mult.rm);
                translated.mult.rs = regmap_load(i.mult.rs);

                if(i.mult.a)
                {
                    if(i.mult.l)
                    {
                        translated.mult.rdlo = regmap_loadstore(i.mult.rdlo);
                        translated.mult.rdhi = regmap_loadstore(i.mult.rdhi);
                    }
                    else
                    {
                        translated.mult.rdlo = regmap_load(i.mult.rdlo);
                        translated.mult.rd = regmap_store(i.mult.rd);
                    }
                }
                else if(i.mult.l)
                {
                    translated.mult.rdlo = regmap_store(i.mult.rdlo);
                    translated.mult.rdhi = regmap_store(i.mult.rdhi);
                }
                else
                    translated.mult.rd = regmap_store(i.mult.rd);

                emit(translated.raw);

                goto instruction_translated;
            }

            if(i.mem_proc2.s || !i.mem_proc2.h)
                goto unimpl; // Signed byte/halfword and doubleword not implemented

            if(i.mem_proc2.rn == PC
                    || i.mem_proc2.rd == PC
                    || i.mem_proc2.rm == PC)
                goto unimpl; // PC as operand or dest. not implemented

            // Load base into r0
            emit_mov_reg(R0, regmap_load(i.mem_proc2.rn));

            regmap_flush();

            // Offset into r5
            if(i.mem_proc2.i) // Immediate offset
                emit_mov(R5, (i.mem_proc2.immed_h << 4) | i.mem_proc2.immed_l);
            else // Register offset
                emit_ldr_armreg(R5, i.mem_proc2.rm);

            // Get final address..
            if(i.mem_proc2.p)
            {
                // ..into r0
                if(i.mem_proc2.u)
                    emit_al(0x0800005); // add r0, r0, r5
                else
                    emit_al(0x0400005); // sub r0, r0, r5

                if(i.mem_proc2.w) // Writeback: final address into rn
                    emit_str_armreg(R0, i.mem_proc2.rn);
            }
            else
            {
                // ..into r5
                if(i.mem_proc2.u)
                    emit_al(0x0805005); // add r5, r0, r5
                else
                    emit_al(0x0405005); // sub r5, r0, r5
            }

            if(i.mem_proc2.l)
            {
                emit_call(reinterpret_cast<void*>(read_half_asm));
                emit_str_armreg(R0, i.mem_proc2.rd);
            }
            else
            {
                emit_ldr_armreg(R1, i.mem_proc2.rd);
                emit_call(reinterpret_cast<void*>(write_half_asm));
            }

            // Post-indexed: final address in r5 back into rn
            if(!i.mem_proc2.p)
                emit_str_armreg(R5, i.mem_proc2.rn);
        }
        else if((insn & 0xD900000) == 0x1000000)
        {
            if((insn & 0xFFFFFD0) == 0x12FFF10)
            {
                //B(L)X
                if(i.bx.rm == PC)
                    goto unimpl;

                // Load destination into R0
                emit_mov_reg(R0, regmap_load(i.bx.rm));

                if(i.bx.l)
                    emit_mov(regmap_store(LR), pc + 4);

                emit_jmp(reinterpret_cast<void*>(translation_next_bx));

                if(i.cond == CC_AL)
                    stop_here = true;
            }
            else
                goto unimpl;
        }
        else if((insn & 0xC000000) == 0x0000000)
        {
            // Data processing:
            // The registers the instruction uses are renamed

            if(i.data_proc.rd == PC)
                goto unimpl;

            // Shortcut for simple register mov (mov rd, rm)
            if((i.raw & 0xFFF0FF0) == 0x1a00000)
            {
                if(i.data_proc.rm == PC)
                {
                    emit_mov(regmap_store(i.data_proc.rd), pc + 8);
                    goto instruction_translated;
                }

                uint8_t rm = regmap_load(i.data_proc.rm);
                uint8_t rd = regmap_store(i.data_proc.rd);
                emit_mov_reg(rd, rm);
                goto instruction_translated;
            }

            bool reg_shift = !i.data_proc.imm && i.data_proc.reg_shift;

            // Using pc is not supported
            if((!i.data_proc.imm && i.data_proc.rm == PC)
                    || (reg_shift && i.data_proc.rs == PC))
                goto unimpl;

            Instruction translated;
            translated.raw = (i.raw & 0xFFFFFFF) | 0xE0000000;

            // Map needed register values:
            // add rd, rn, rm, lsl rs becomes add r4, r0, r1, lsl r2 for example

            // MOV and MVN don't have Rn
            if(i.data_proc.op != OP_MOV && i.data_proc.op != OP_MVN)
            {
                if(i.data_proc.rn != PC)
                    translated.data_proc.rn = regmap_load(i.data_proc.rn);
                else
                {
                    emit_mov(R0, pc + 8);
                    translated.data_proc.rn = R0;
                }
            }

            // rm is stored in the immediate, don't overwrite it
            if(!i.data_proc.imm)
                translated.data_proc.rm = regmap_load(i.data_proc.rm);

            // Only load rs if actually needed
            if(reg_shift)
                translated.data_proc.rs = regmap_load(i.data_proc.rs);

            // Only change the destination register if it has one
            if(i.data_proc.op < OP_TST || i.data_proc.op > OP_CMN)
                translated.data_proc.rd = regmap_store(i.data_proc.rd);

            // Emit the instruction itself
            emit(translated.raw);
        }
        else if((i.raw & 0xFF000F0) == 0x7F000F0)
            goto unimpl; // undefined
        else if((insn & 0xC000000) == 0x4000000)
        {
            // Memory access: LDR, STRB, etc.

            // User mode access not implemented
            if(!i.mem_proc.p && i.mem_proc.w)
                goto unimpl;

            if(i.mem_proc.rn == PC && !i.mem_proc.p)
                goto unimpl; // Writeback into PC not implemented

            bool offset_is_zero = !i.mem_proc.not_imm && i.mem_proc.immed == 0;

            // Base register gets in r0
            if(i.mem_proc.rn != PC)
                emit_mov_reg(R0, regmap_load(i.mem_proc.rn));
            else if(i.mem_proc.not_imm)
                emit_mov(R0, pc + 8);
            else // Address known
            {
                int offset = i.mem_proc.u ? i.mem_proc.immed :
                                            -i.mem_proc.immed;

                emit_mov(R0, pc + 8 + offset);
                offset_is_zero = true;
            }

            regmap_flush();

            // Skip offset calculation
            if(offset_is_zero)
                goto no_offset;

            // Offset gets in r5
            if(!i.mem_proc.not_imm)
            {
                // Immediate offset
                emit_mov(R5, i.mem_proc.immed);
            }
            else
            {
                // Shifted register: translate to mov
                Instruction off;
                off.raw = 0xe1a05000; // mov r5, something
                off.raw |= insn & 0xFFF; // Copy shifter_operand
                off.data_proc.rm = R1;
                if(i.mem_proc.rm != PC)
                    emit_ldr_armreg(R1, i.mem_proc.rm);
                else
                    emit_mov(R1, pc + 8);

                emit(off.raw);
            }

            // Get final address..
            if(i.mem_proc.p)
            {
                // ..in r0
                if(i.mem_proc.u)
                    emit_al(0x0800005); // add r0, r0, r5
                else
                    emit_al(0x0400005); // sub r0, r0, r5

                // It has to be stored AFTER the memory access, to be able
                // to perform the access again after an abort.
                if(i.mem_proc.w) // Writeback: final address into rn
                    emit_mov_reg(R5, R0);
            }
            else
            {
                // ..in r5
                if(i.mem_proc.u)
                    emit_al(0x0805005); // add r5, r0, r5
                else
                    emit_al(0x0405005); // sub r5, r0, r5
            }

            no_offset:
            if(i.mem_proc.l)
            {
                emit_call(reinterpret_cast<void*>(i.mem_proc.b ? read_byte_asm : read_word_asm));
                if(i.mem_proc.rd != PC)
                    emit_str_armreg(R0, i.mem_proc.rd); // r0 is return value
            }
            else
            {
                if(i.mem_proc.rd != PC)
                    emit_ldr_armreg(R1, i.mem_proc.rd); // r1 is the value
                else
                    emit_mov(R1, pc + 12);

                emit_call(reinterpret_cast<void*>(i.mem_proc.b ? write_byte_asm : write_word_asm));
            }

            // Post-indexed: final address from r5 into rn
            if(!offset_is_zero && (!i.mem_proc.p || i.mem_proc.w))
                emit_str_armreg(R5, i.mem_proc.rn);

            // Jump after writeback, to support post-indexed jumps
            if(i.mem_proc.l && i.mem_proc.rd == PC)
            {
                // pc is destination register
                emit_jmp(reinterpret_cast<void*>(translation_next_bx));
                // It's an unconditional jump
                if(i.cond == CC_AL)
                    jumps_away = stop_here = true;
            }
        }
        else if((insn & 0xE000000) == 0x8000000)
        {
            if(i.mem_multi.s)
                goto unimpl; // Exception return or usermode not implemented

            if(i.mem_multi.rn == PC)
                goto unimpl;

            uint16_t reglist = i.mem_multi.reglist;
            if(reglist & (1 << i.mem_multi.rn))
                goto unimpl; // Loading/Saving address register

            regmap_flush();
            emit_str_flags();

            int count = __builtin_popcount(reglist), offset, wb_offset;

            if(i.mem_multi.u) // Increment
            {
                wb_offset = count * 4;
                offset = 0;
            }
            else // Decrement
            {
                wb_offset = count * -4;
                offset = wb_offset;
            }

            if(i.mem_multi.p == i.mem_multi.u)
                offset += 4;

            // Base reg in r4
            emit_ldr_armreg(R4, i.mem_multi.rn);

            for(unsigned int reg = 0; reglist; reglist >>= 1, reg++)
            {
                if((reglist & 1) == 0)
                    continue;

                if(offset >= 0)
                    emit_al(0x2840000 | offset); // add r0, r4, #offset
                else
                    emit_al(0x2440000 | -offset); // sub r0, r4, #-offset

                if(i.mem_multi.l)
                {
                    emit_call(reinterpret_cast<void*>(read_word_asm), false);
                    if(reg != PC)
                        emit_str_armreg(R0, reg);
                    //else written below
                }
                else
                {
                    if(reg == PC)
                        emit_mov(R1, pc + 12);
                    else
                        emit_ldr_armreg(R1, reg);

                    emit_call(reinterpret_cast<void*>(write_word_asm), false);
                }

                offset += 4;
            }

            if(i.mem_multi.w)
            {
                if(wb_offset >= 0)
                    emit_al(0x2841000 | wb_offset); // add r1, r4, #wb_offset
                else
                    emit_al(0x2441000 | -wb_offset); // sub r1, r4, #-wb_offset

                emit_str_armreg(R1, i.mem_multi.rn);
            }

            if(i.mem_multi.l && (i.mem_multi.reglist & (1 << PC))) // Loading PC
            {
                // PC already in R0 (last one loaded)
                emit_jmp(reinterpret_cast<void*>(translation_next_bx), false);
                if(i.cond == CC_AL)
                    jumps_away = stop_here = true;
            }

            emit_ldr_flags();
        }
        else if((insn & 0xE000000) == 0xA000000)
        {
            /* Branches work this way:
             * Either jump to translation_next if code not translated (yet) or
             * jump directly to the translated code, over a small function checking for pending events */

            uint32_t addr = pc + ((int32_t)(i.raw << 8) >> 6) + 8;

            if(i.branch.l)
            {
                // Save return address in LR
                emit_mov(regmap_store(LR), pc + 4);
            }
            /* If the control flow diverges, end this block.
               The condition for this is that the branch is unconditional
               and the branch target doesn't point to the next 4 instructions. */
            else if(i.cond == CC_AL && (addr <= pc || addr - pc > 16))
            {
                // It's not likely that the branch will return
                jumps_away = stop_here = true;
            }

            const uint32_t *ptr = reinterpret_cast<uint32_t*>(try_ptr(addr));

            if(ptr == nullptr)
            {
                // Not readable, use translation_next
                emit_mov(R0, addr);
                emit_jmp(reinterpret_cast<void*>(translation_next));
            }
            else if (!(RAM_FLAGS(ptr) & RF_CODE_TRANSLATED))
            {
                // Not translated - use translation_jmp_ptr(ptr)
                emit_mov(R0, addr);
                emit_str_armreg(R0, PC);
                emit_mov(R0, uintptr_t(ptr));
                emit_jmp(reinterpret_cast<void*>(translation_jmp_ptr));
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
        {
            unimpl:
            // There may be a partial translation in memory, scrap it.
            translate_current = translate_buffer_inst_start;
            RAM_FLAGS(insn_ptr) |= RF_CODE_NO_TRANSLATE;

            break;
        }

        instruction_translated:

        if(cond_branch)
        {
            // The code up until this point might never get executed, so regmaps won't have any effect
            regmap_flush();

            // Fixup the branch above (-2 to account for the pipeline)
            *cond_branch |= (translate_current - cond_branch - 2) & 0xFFFFFF;
            cond_branch = nullptr;
        }

        if(can_jump_here)
            RAM_FLAGS(insn_ptr) |= (RF_CODE_TRANSLATED | next_translation_index << RFS_TRANSLATION_INDEX);
        // else just don't set it. When the CPU jumps to it, it'll be treated as new basic block

        ++jump_table_current;
        ++insn_ptr;
        pc += 4;
    }

    // Did we do any translation at all?
    if(insn_ptr == insn_ptr_start)
    {
        #ifdef IS_IOS_BUILD
            // Mark translate_buffer as R_X
            // Even if no translation was done, pages got marked RW_
            mprotect(translate_buffer, INSN_BUFFER_SIZE, PROT_READ | PROT_EXEC);
        #endif

        // Apparently not.
        translate_current = translate_buffer_inst_start;
        return;
    }

    if(!jumps_away)
    {
        // Same code as for relative branches
        uint32_t *ptr = reinterpret_cast<uint32_t*>(try_ptr(pc));

        if(ptr == nullptr)
        {
            // Not translated, use translation_next
            emit_mov(R0, pc);
            emit_jmp(reinterpret_cast<void*>(translation_next));
        }
        else if (!(RAM_FLAGS(ptr) & RF_CODE_TRANSLATED))
        {
            emit_mov(R0, pc);
            emit_str_armreg(R0, PC);
            emit_mov(R0, uintptr_t(ptr));
            emit_jmp(reinterpret_cast<void*>(translation_jmp_ptr));
        }
        else
        {
            // Get address of translated code to jump to it
            translation *target_translation = &translation_table[RAM_FLAGS(ptr) >> RFS_TRANSLATION_INDEX];
            uintptr_t jmp_target = reinterpret_cast<uintptr_t>(target_translation->jump_table[ptr - target_translation->start_ptr]);

            // Update pc first
            emit_mov(R0, pc);
            emit_str_armreg(R0, PC);
            emit_mov(R0, jmp_target);
            emit_jmp(reinterpret_cast<void*>(translation_jmp));
        }
    }

    #ifdef IS_IOS_BUILD
        // Mark translate_buffer as R_X
        mprotect(translate_buffer, INSN_BUFFER_SIZE, PROT_READ | PROT_EXEC);
    #endif
    
    // Only flush cache until the literal pool
    uint32_t *code_end = translate_current;
    // Emit the literal pool
    literalpool_fill();

    this_translation->end_ptr = insn_ptr;
    this_translation->unused = reinterpret_cast<uintptr_t>(translate_current);

    //dump_translation(next_translation_index);

    // This effectively flushes this_translation, as it won't get used next time
    next_translation_index += 1;

    // Flush the instruction cache
#ifdef IS_IOS_BUILD
    sys_cache_control(1 /* kCacheFunctionPrepareForExecution */, jump_table_start[0], (code_end-jump_table_start[0])*4);
#else
    __builtin___clear_cache((char*)jump_table_start[0], (char*)code_end);
#endif
}

static void _invalidate_translation(int index)
{
    /* Disabled for now. write_action not called in asmcode_arm.S so this can't happen
       and translation_pc_ptr is inaccurate due to translation_jmp anyway.

    uint32_t flags = RAM_FLAGS(translation_pc_ptr);
    if ((flags & RF_CODE_TRANSLATED) && (int)(flags >> RFS_TRANSLATION_INDEX) == index)
        error("Cannot modify currently executing code block.");
    */

    uint32_t *start = translation_table[index].start_ptr;
    uint32_t *end   = translation_table[index].end_ptr;
    for (; start < end; start++)
        RAM_FLAGS(start) &= ~(RF_CODE_TRANSLATED | (~0u << RFS_TRANSLATION_INDEX));
}

void flush_translations()
{
    for(unsigned int index = 0; index < next_translation_index; index++)
        _invalidate_translation(index);

    next_translation_index = 0;
    translate_current = translate_buffer;
    jump_table_current = jump_table;
}

void invalidate_translation(int index)
{
    /* Due to translation_jmp using absolute pointers in the JIT, we can't just
       invalidate a single translation. */
    #ifdef SUPPORT_LINUX
        flush_translations();
    #else
        _invalidate_translation(index);
    #endif
}

void translate_fix_pc()
{
    if (!translation_sp)
        return;

    // This is normally done when leaving the translation,
    // but since we are here, this didn't happen (longjmp)
    set_cpsr_flags(arm.cpsr_flags);

    uint32_t *insnp = reinterpret_cast<uint32_t*>(try_ptr(arm.reg[15]));
    uint32_t flags = 0;
    if(!insnp || !((flags = RAM_FLAGS(insnp)) & RF_CODE_TRANSLATED))
        return error("Couldn't get PC for fault");

    int index = flags >> RFS_TRANSLATION_INDEX;
    assert(insnp >= translation_table[index].start_ptr);
    assert(insnp < translation_table[index].end_ptr);

    void *ret_pc = translation_sp[-1];

    unsigned int jump_index = insnp - translation_table[index].start_ptr;
    unsigned int translation_insts = translation_table[index].end_ptr - translation_table[index].start_ptr;

    for(unsigned int i = jump_index; ret_pc > translation_table[index].jump_table[i] && i < translation_insts; ++i)
        arm.reg[15] += 4;

    cycle_count_delta -= translation_table[index].end_ptr - insnp;
    translation_sp = nullptr;
}
