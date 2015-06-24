#include <assert.h>

#include "emu.h"
#include "mem.h"
#include "cpu.h"
#include "asmcode.h"
#include "translate.h"
#include "debug.h"
#include "os/os.h"

extern void translation_enter() __asm__("translation_enter");
extern void translation_next() __asm__("translation_next");
extern void translation_next_bx() __asm__("translation_next_bx");
extern uintptr_t arm_shift_proc[2][4] __asm__("arm_shift_proc");
void **in_translation_rsp __asm__("in_translation_rsp");
void *in_translation_pc_ptr __asm__("in_translation_pc_ptr");

#define MAX_TRANSLATIONS 262144
struct translation translation_table[MAX_TRANSLATIONS];

static int next_index = 0;
uint8_t *insn_buffer = NULL;
uint8_t *insn_bufptr = NULL;
static uint8_t *jtbl_buffer[500000];
static uint8_t **jtbl_bufptr = jtbl_buffer;
static uint8_t *out;
static uint8_t **outj;

#define REG_ARG1 EDI
#define REG_ARG2 ESI

enum x86_reg { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI };
enum x86_reg8 { AL, CL, DL, BL, AH, CH, DH, BH };
enum group1 { ADD, OR, ADC, SBB, AND, SUB, XOR, CMP };
enum group2 { ROL, ROR, RCL, RCR, SHL, SHR, SAL, SAR };
enum group3 { NOT = 2, NEG, MUL, IMUL, DIV, IDIV };

/* x86 conditional jump instructions */
enum { JO = 0x70, JNO, JB,  JAE, JZ, JNZ, JBE, JA,
       JS = 0x78, JNS, JPE, JPO, JL, JGE, JLE, JG };

static inline void emit_byte(uint8_t b)    { *out++ = b; }
static inline void emit_word(uint16_t w)   { *(uint16_t *)out = w; out += 2; }
static inline void emit_dword(uint32_t dw) { *(uint32_t *)out = dw; out += 4; }

/*This is a hack:
 * -regs not saved
 * -stack not aligned */
static inline void emit_call_nosave(uintptr_t target) {
    emit_byte(0xE8);
    int64_t diff = target - ((uintptr_t) out + 4);
    if(diff > INT32_MAX || diff < INT32_MIN)
        assert(false); //Distance doesn't fit into immediate

    emit_dword(diff);
}

//The AMD64 ABI says that most regs have to be saved by the caller
static inline void emit_call(uintptr_t target) {
    //If you change the stack layout, change the usage of in_translation_rsp in translate_fix_pc below as well!

    //TODO: Verify that %rdi isn't that important to save (it's the first arg)
    //emit_byte(0x57); // push %rdi
    emit_byte(0x56); // push %rsi
    emit_byte(0x52); // push %rdx
    emit_byte(0x51); // push %rcx

    emit_call_nosave(target);

    emit_byte(0x59);
    emit_byte(0x5a);
    emit_byte(0x5e);
    //emit_byte(0x5f);
}

static inline void emit_jump(uintptr_t target) {
    emit_byte(0xE9);
    int64_t diff = target - ((uintptr_t) out + 4);
    if(diff > INT32_MAX || diff < INT32_MIN)
        assert(false);

    emit_dword(diff);
}

// ----------------------------------------------------------------------

static inline void emit_modrm_x86reg(int r, int x86reg) {
    emit_byte(0xC0 | r << 3 | x86reg);
}

static void emit_modrm_base_offset(int r, int basex86reg, int offset) {
    if (offset == 0) {
        emit_byte(basex86reg | r << 3);
    } else if (offset >= -0x80 && offset < 0x80) {
        emit_byte(0x40 | basex86reg | r << 3);
        emit_byte(offset);
    } else {
        emit_byte(0x80 | basex86reg | r << 3);
        emit_dword(offset);
    }
}

static void emit_modrm_armreg(int r, int armreg) {
    if (armreg < 0 || armreg > 14) error("translation f***up");
    emit_modrm_base_offset(r, EBX, (uint8_t *)&arm.reg[armreg] - (uint8_t *)&arm);
}

// ----------------------------------------------------------------------

static void emit_mov_x86reg_immediate(int x86reg, int imm) {
    emit_byte(0xB8 | x86reg);
    emit_dword(imm);
}

static void emit_alu_x86reg_immediate(int aluop, int x86reg, int imm) {
    if (imm >= -0x80 && imm < 0x80) {
        emit_byte(0x83);
        emit_modrm_x86reg(aluop, x86reg);
        emit_byte(imm);
    } else if (x86reg == EAX) {
        emit_byte(0x05 | aluop << 3);
        emit_dword(imm);
    } else {
        emit_byte(0x81);
        emit_modrm_x86reg(aluop, x86reg);
        emit_dword(imm);
    }
}

static void emit_mov_armreg_immediate(int armreg, int imm) {
    emit_byte(0xC7);
    emit_modrm_armreg(0, armreg);
    emit_dword(imm);
}

static void emit_alu_armreg_immediate(int aluop, int armreg, int imm) {
    if (imm >= -0x80 && imm < 0x80) {
        emit_byte(0x83);
        emit_modrm_armreg(aluop, armreg);
        emit_byte(imm);
    } else {
        emit_byte(0x81);
        emit_modrm_armreg(aluop, armreg);
        emit_dword(imm);
    }
}

static inline void emit_mov_x86reg_x86reg(int dest, int src) {
    emit_byte(0x8B);
    emit_modrm_x86reg(dest, src);
}

static inline void emit_alu_x86reg_x86reg(int aluop, int dest, int src) {
    emit_byte(0x03 | aluop << 3);
    emit_modrm_x86reg(dest, src);
}

static inline void emit_mov_x86reg_armreg(int x86reg, int armreg) {
    emit_byte(0x8B);
    emit_modrm_armreg(x86reg, armreg);
}

static inline void emit_alu_x86reg_armreg(int aluop, int x86reg, int armreg) {
    emit_byte(0x03 | aluop << 3);
    emit_modrm_armreg(x86reg, armreg);
}

static inline void emit_mov_armreg_x86reg(int armreg, int x86reg) {
    emit_byte(0x89);
    emit_modrm_armreg(x86reg, armreg);
}

static inline void emit_alu_armreg_x86reg(int aluop, int armreg, int x86reg) {
    emit_byte(0x01 | aluop << 3);
    emit_modrm_armreg(x86reg, armreg);
}

static inline void emit_unary_x86reg(int unop, int x86reg) {
    emit_byte(0xF7);
    emit_modrm_x86reg(unop, x86reg);
}

static inline void emit_unary_armreg(int unop, int armreg) {
    emit_byte(0xF7);
    emit_modrm_armreg(unop, armreg);
}

static inline void emit_test_armreg_immediate(int armreg, int imm) {
    emit_byte(0xF7);
    emit_modrm_armreg(0, armreg);
    emit_dword(imm);
}

static inline void emit_test_armreg_x86reg(int armreg, int x86reg) {
    emit_byte(0x85);
    emit_modrm_armreg(x86reg, armreg);
}

static inline void emit_test_x86reg_x86reg(int reg1, int reg2) {
    emit_byte(0x85);
    emit_modrm_x86reg(reg1, reg2);
}

#define SHIFT_BY_CL -1
static void emit_shift_x86reg(int shiftop, int x86reg, int count) {
    if (count == SHIFT_BY_CL) {
        emit_byte(0xD3);
        emit_modrm_x86reg(shiftop, x86reg);
    } else if (count == 0) {
        /* no-op */
    } else if (count == 1) {
        emit_byte(0xD1);
        emit_modrm_x86reg(shiftop, x86reg);
    } else {
        emit_byte(0xC1);
        emit_modrm_x86reg(shiftop, x86reg);
        emit_byte(count);
    }
}

static void emit_shift_armreg(int shiftop, int armreg, int count) {
    if (count == SHIFT_BY_CL) {
        emit_byte(0xD3);
        emit_modrm_armreg(shiftop, armreg);
    } else if (count == 0) {
        /* no-op */
    } else if (count == 1) {
        emit_byte(0xD1);
        emit_modrm_armreg(shiftop, armreg);
    } else {
        emit_byte(0xC1);
        emit_modrm_armreg(shiftop, armreg);
        emit_byte(count);
    }
}

static inline void emit_mov_x86reg8_immediate(int x86reg, int immediate) {
    emit_byte(0xB0 | x86reg);
    emit_byte(immediate);
}
static inline void emit_cmp_flag_immediate(void *flagptr, int immediate) {
    emit_byte(0x80);
    emit_modrm_base_offset(CMP, EBX, (uint8_t *)flagptr - (uint8_t *)&arm);
    emit_byte(immediate);
}
static inline void emit_mov_x86reg8_flag(int x86reg, void *flagptr) {
    emit_byte(0x8A);
    emit_modrm_base_offset(x86reg, EBX, (uint8_t *)flagptr - (uint8_t *)&arm);
}
static inline void emit_alu_x86reg8_flag(int aluop, int x86reg, void *flagptr) {
    emit_byte(0x02 | aluop << 3);
    emit_modrm_base_offset(x86reg, EBX, (uint8_t *)flagptr - (uint8_t *)&arm);
}
static inline void emit_mov_flag_immediate(void *flagptr, int imm) {
    emit_byte(0xC6);
    emit_modrm_base_offset(0, EBX, (uint8_t *)flagptr - (uint8_t *)&arm);
    emit_byte(imm);
}
enum { SETO = 0x90, SETNO, SETB,  SETAE, SETZ, SETNZ, SETBE, SETA,
       SETS,        SETNS, SETPE, SETPO, SETL, SETGE, SETLE, SETG };
static inline void emit_setcc_flag(int setcc, void *flagptr) {
    emit_byte(0x0F);
    emit_byte(setcc);
    emit_modrm_base_offset(0, EBX, (uint8_t *)flagptr - (uint8_t *)&arm);
}

bool translate_init()
{
    if(!insn_buffer)
    {
        insn_buffer = os_alloc_executable(INSN_BUFFER_SIZE);
        insn_bufptr = insn_buffer;
    }

    return !!insn_buffer;
}

void translate_deinit()
{
    if(!insn_buffer)
        return;

    os_free(insn_buffer, INSN_BUFFER_SIZE);
    insn_buffer = NULL;
}

void translate(uint32_t start_pc, uint32_t *start_insnp) {
    out = insn_bufptr;
    outj = jtbl_bufptr;
    uint32_t pc = start_pc;
    uint32_t *insnp = start_insnp;

    if (next_index >= MAX_TRANSLATIONS)
        error("too many translations");

    uint8_t *insn_start;
    int stop_here = 0;
    while (1) {
        if (out >= &insn_buffer[INSN_BUFFER_SIZE - 1000])
            error("Out of instruction space");
        if (outj >= &jtbl_buffer[sizeof jtbl_buffer / sizeof *jtbl_buffer])
            error("Out of jump table space");

        insn_start = out;

        if ((pc ^ start_pc) & ~0x3FF) {
            //printf("stopping translation - end of page\n");
            goto branch_conditional;
        }
        if (RAM_FLAGS(insnp) & (RF_EXEC_BREAKPOINT | RF_EXEC_DEBUG_NEXT | RF_EXEC_HACK | RF_CODE_TRANSLATED | RF_CODE_NO_TRANSLATE)) {
            //printf("stopping translation - at breakpoint %x (%x)\n", pc);
            goto branch_conditional;
        }
        uint32_t insn = *insnp;

        /* Condition code */
        int cond = insn >> 28;
        int jcc = JZ;
        uint8_t *cond_jmp_offset = NULL;
        switch (cond >> 1) {
            case 0: /* EQ (Z), NE (!Z) */
                emit_cmp_flag_immediate(&arm.cpsr_z, 0);
                break;
            case 1: /* CS (C), CC (!C) */
                emit_cmp_flag_immediate(&arm.cpsr_c, 0);
                break;
            case 2: /* MI (N), PL (!N) */
                emit_cmp_flag_immediate(&arm.cpsr_n, 0);
                break;
            case 3: /* VS (V), VC (!V) */
                emit_cmp_flag_immediate(&arm.cpsr_v, 0);
                break;
            case 4: /* HI (!Z & C), LS (Z | !C) */
                emit_mov_x86reg8_flag(AL, &arm.cpsr_z);
                emit_alu_x86reg8_flag(CMP, AL, &arm.cpsr_c);
                jcc = JAE; // execute if Z is less than C
                break;
            case 5: /* GE (N = V), LT (N != V) */
                emit_mov_x86reg8_flag(AL, &arm.cpsr_n);
                emit_alu_x86reg8_flag(CMP, AL, &arm.cpsr_v);
                jcc = JNZ;
                break;
            case 6: /* GT (!Z & N = V), LE (Z | N != V) */
                emit_mov_x86reg8_flag(AL, &arm.cpsr_n);
                emit_alu_x86reg8_flag(XOR, AL, &arm.cpsr_v);
                emit_alu_x86reg8_flag(OR, AL, &arm.cpsr_z);
                jcc = JNZ;
                break;
            case 7: /* AL */
                if (cond & 1) goto unimpl;
                goto no_condition;
        }
        /* If condition not met, jump around code.
         * (If ARM condition code is inverted, invert x86 code too) */
        emit_byte(jcc ^ (cond & 1));
        emit_byte(0);
        cond_jmp_offset = out;
no_condition:

        if ((insn & 0xE000090) == 0x0000090) {
            if ((insn & 0xFC000F0) == 0x0000090) {
                /* MUL, MLA - 32x32->32 multiplications */
                int left_reg  = insn & 15;
                int right_reg = insn >> 8 & 15;
                int acc_reg   = insn >> 12 & 15;
                int dest_reg  = insn >> 16 & 15;
                if (left_reg == 15 || right_reg == 15 || acc_reg == 15 || dest_reg == 15)
                    goto unimpl;

                emit_mov_x86reg_armreg(EAX, left_reg);
                emit_unary_armreg(MUL, right_reg);
                if (insn & 0x0200000)
                    emit_alu_x86reg_armreg(ADD, EAX, acc_reg);
                emit_mov_armreg_x86reg(dest_reg, EAX);

                if (insn & 0x0100000) {
                    if (!(insn & 0x0200000))
                        emit_test_x86reg_x86reg(EAX, EAX);
                    emit_setcc_flag(SETS, &arm.cpsr_n);
                    emit_setcc_flag(SETZ, &arm.cpsr_z);
                }
            } else if ((insn & 0xF8000F0) == 0x0800090) {
                /* UMULL, UMLAL, SMULL, SMLAL: 32x32 to 64 multiplications */
                uint32_t left_reg  = insn & 15;
                uint32_t right_reg = insn >> 8  & 15;
                uint32_t reg_lo    = insn >> 12 & 15;
                uint32_t reg_hi    = insn >> 16 & 15;

                if (left_reg == 15 || right_reg == 15 || reg_lo == 15 || reg_hi == 15)
                    goto unimpl;
                if (reg_lo == reg_hi)
                    goto unimpl;
                if (insn & 0x0100000) // set flags
                    goto unimpl;

                emit_mov_x86reg_armreg(EAX, left_reg);
                emit_unary_armreg((insn & 0x0400000) ? IMUL : MUL, right_reg);
                if (insn & 0x0200000) {
                    /* Accumulate */
                    emit_alu_armreg_x86reg(ADD, reg_lo, EAX);
                    emit_alu_armreg_x86reg(ADC, reg_hi, EDX);
                } else {
                    emit_mov_armreg_x86reg(reg_lo, EAX);
                    emit_mov_armreg_x86reg(reg_hi, EDX);
                }
            } else {
                enum { INVALID, H, SB, SH } type;
                int is_load = insn & (1 << 20);
                type = insn >> 5 & 3;
                if (type == INVALID || (!is_load && type != H))
                    // multiply, SWP, or doubleword access
                    goto unimpl;

                int post_index = !(insn & (1 << 24));
                int offset_op = (insn & (1 << 23)) ? ADD : SUB;
                int pre_index = insn & (1 << 21);
                int base_reg = insn >> 16 & 15;
                int data_reg = insn >> 12 & 15;

                if (base_reg == 15 || data_reg == 15)
                    goto unimpl;

                if (pre_index || post_index) {
                    if (pre_index && post_index) goto unimpl;
                    if (base_reg == 15) goto unimpl;
                    if (is_load && base_reg == data_reg) goto unimpl;
                }

                if (insn & (1 << 22)) {
                    // Offset is immediate
                    int offset = (insn & 0x0F) | (insn >> 4 & 0xF0);
                    emit_mov_x86reg_armreg(REG_ARG1, base_reg);
                    if (!post_index && offset != 0)
                        emit_alu_x86reg_immediate(offset_op, REG_ARG1, offset);
                } else {
                    // Offset is register
                    int offset_reg = insn & 0x0F;
                    if (offset_reg == 15)
                        goto unimpl;
                    if (post_index || pre_index)
                        goto unimpl;
                    emit_mov_x86reg_armreg(REG_ARG1, base_reg);
                    emit_alu_x86reg_armreg(offset_op, REG_ARG1, offset_reg);
                }

                if (is_load) {
                    if (type == SB) {
                        emit_call((uintptr_t)read_byte);
                        // movsx eax,al
                        emit_word(0xBE0F);
                        emit_byte(0xC0);
                    } else {
                        emit_call((uintptr_t)read_half);
                        if (type == SH) {
                            // cwde
                            emit_byte(0x98);
                        }
                    }
                    emit_mov_armreg_x86reg(data_reg, EAX);
                } else {
                    emit_mov_x86reg_armreg(REG_ARG2, data_reg);
                    emit_call((uintptr_t)write_half);
                }

                if (post_index || pre_index)
                    emit_alu_armreg_immediate(offset_op, base_reg, ((insn & 0x0F) | (insn >> 4 & 0xF0)));
            }
        } else if ((insn & 0xD900000) == 0x1000000) {
            if ((insn & 0xFFFFFD0) == 0x12FFF10) {
                /* BX/BLX */
                int target_reg = insn & 15;
                if (target_reg == 15)
                    break;
                emit_mov_x86reg_armreg(EAX, target_reg);
                if (insn & 0x20)
                    emit_mov_armreg_immediate(14, pc + 4);
                emit_jump((uintptr_t)translation_next_bx);
                stop_here = 1;
            } else if ((insn & 0xFBF0FFF) == 0x10F0000) {
                /* MRS - move reg <- status */
                int target_reg = insn >> 12 & 15;
                if (target_reg == 15)
                    break;
                emit_call((insn & 0x0400000) ? (uintptr_t)get_spsr : (uintptr_t)get_cpsr);
                emit_mov_armreg_x86reg(target_reg, EAX);
            } else if ((insn & 0xFB0FFF0) == 0x120F000 ||
                       (insn & 0xFB0F000) == 0x320F000) {
                /* MSR - move status <- reg/imm */
                uint32_t mask = 0;
                if (insn & 0x2000000) {
                    uint32_t imm = insn & 0xFF;
                    int rotate = insn >> 7 & 30;
                    imm = imm >> rotate | imm << (32 - rotate);
                    emit_mov_x86reg_immediate(REG_ARG1, imm);
                } else {
                    int reg = insn & 15;
                    if (reg == 15)
                        break;
                    emit_mov_x86reg_armreg(REG_ARG1, reg);
                }
                if (insn & 0x0080000) mask |= 0xFF000000;
                if (insn & 0x0040000) mask |= 0x00FF0000;
                if (insn & 0x0020000) mask |= 0x0000FF00;
                if (insn & 0x0010000) mask |= 0x000000FF;
                emit_mov_x86reg_immediate(REG_ARG2, mask);
                emit_call((insn & 0x0400000) ? (uintptr_t)set_spsr : (uintptr_t)set_cpsr);
                // If cpsr_c changed, leave translation to check for interrupts
                if ((insn & 0x0410000) == 0x0010000) {
                    emit_mov_x86reg_immediate(EAX, pc + 4);
                    emit_jump((uintptr_t)translation_next);
                }
            } else if ((insn & 0xFFF0FF0) == 0x16F0F10) {
                /* CLZ: Count leading zeros */
                int src_reg = insn & 15;
                int dst_reg = insn >> 12 & 15;
                if (src_reg == 15 || dst_reg == 15)
                    break;
                emit_word(0xBD0F); // BSR
                emit_modrm_armreg(EAX, src_reg);
                emit_word(5 << 8 | JNZ);
                emit_mov_x86reg_immediate(EAX, 63);
                emit_alu_x86reg_immediate(XOR, EAX, 31);
                emit_mov_armreg_x86reg(dst_reg, EAX);
            } else {
                break;
            }
        } else if ((insn & 0xC000000) == 0) {
            /* Data processing instructions */
            int right_reg = insn & 15;
            int dest_reg = insn >> 12 & 15;
            int left_reg = insn >> 16 & 15;
            int setcc = insn >> 20 & 1;
            int op = insn >> 21 & 15;

            if (dest_reg == 15 || left_reg == 15)
                break; // not dealing with this for now

            int set_overflow = -1;
            int set_carry = -1;
            int right_is_imm = insn >> 25 & 1;
            int right_is_reg = 0;
            uint32_t imm = 0; // value not used, just suppressing uninitialized variable warning
            if (right_is_imm) {
                // Right operand is immediate
                imm = insn & 0xFF;
                int rotate = insn >> 7 & 30;
                imm = imm >> rotate | imm << (32 - rotate);
                if (rotate != 0)
                    set_carry = imm >> 31;
            } else if (right_reg == 15) {
                if (insn & 0xFF0) // Shifted PC?! Not likely.
                    goto unimpl;
                imm = pc + 8;
                right_is_imm = 1;
            } else {
                int shift_type = insn >> 5 & 3;
                static const uint8_t shift_table[] = { SHL, SHR, SAR, ROR };
                int x86_shift_type = shift_table[shift_type];

                int count = insn >> 7 & 31;
                int shift_need_carry = setcc & ((0xF303 >> op) & 1);
                if (insn & (1 << 4)) {
                    if (insn & (1 << 7))
                        goto unimpl;
                    /* Register shifted by register.
                     * ARM's shifts are very different from x86's, unfortunately.
                     * In x86, only 5 bits of the shift count are used.
                     * In ARM, 8 bits are used. To implement ARM shifts on x86,
                     * one must check for the 32-255 cases explicitly.
                     * This is done in asmcode.S */

                    int shift_reg = count >> 1;
                    if (shift_reg == 15)
                        goto unimpl;

                    emit_mov_x86reg_armreg(ECX, shift_reg);
                    if (shift_type == 3 && !shift_need_carry) {
                        /* Ignoring flags, ARM's ROR is the same as x86's :) */
                        count = SHIFT_BY_CL;
                        goto simple_shift;
                    }

                    emit_mov_x86reg_armreg(EAX, right_reg);
                    emit_call_nosave(arm_shift_proc[shift_need_carry][shift_type]);

                    shift_need_carry = 0; /* Already set by the function */
                } else if (count == 0) {
                    if (shift_type == 0) {
                        /* Right operand is just an ARM register */
                        right_is_reg = 1;
                        shift_need_carry = 0;
                    } else if (shift_type == 1) {
                        /* LSR #32 */
                        if (shift_need_carry) {
                            emit_mov_x86reg_armreg(EAX, right_reg);
                            emit_shift_x86reg(SHL, EAX, 1);
                        }
                        imm = 0;
                        right_is_imm = 1;
                    } else if (shift_type == 2) {
                        /* ASR #32 */
                        emit_mov_x86reg_armreg(EAX, right_reg);
                        emit_shift_x86reg(SAR, EAX, 31);
                        if (shift_need_carry)
                            emit_shift_x86reg(SAR, EAX, 1);
                    } else if (shift_type == 3) {
                        /* RRX */
                        emit_mov_x86reg8_immediate(AL, 0);
                        emit_alu_x86reg8_flag(CMP, AL, &arm.cpsr_c);
                        x86_shift_type = RCR;
                        count = 1;
                        goto simple_shift;
                    }
                } else {
simple_shift:
                    if (dest_reg == right_reg && op == 13) {
                        /* MOV of a shifted register to itself. Do shift in-place */
                        emit_shift_armreg(x86_shift_type, dest_reg, count);
                        right_is_reg = 1;
                    } else {
                        emit_mov_x86reg_armreg(EAX, right_reg);
                        emit_shift_x86reg(x86_shift_type, EAX, count);
                    }
                }
                if (shift_need_carry)
                    emit_setcc_flag(SETB, &arm.cpsr_c);
            }

            if (op == 13 || op == 15) {
                if (right_is_imm) {
                    if (op == 15)
                        imm = ~imm;
                    emit_mov_armreg_immediate(dest_reg, imm);
                    if (setcc)
                        goto unimpl;
                } else if (right_is_reg && dest_reg == right_reg) {
                    /* MOV/MVN of a register to itself */
                    if (op == 15) {
                        if (setcc)
                            emit_alu_armreg_immediate(XOR, dest_reg, -1);
                        else
                            emit_unary_armreg(NOT, dest_reg);
                    } else {
                        if (setcc)
                            emit_alu_armreg_immediate(CMP, dest_reg, 0);
                    }
                } else {
                    if (right_is_reg)
                        emit_mov_x86reg_armreg(EAX, right_reg);
                    if (op == 15)
                        emit_unary_x86reg(NOT, EAX);
                    emit_mov_armreg_x86reg(dest_reg, EAX);
                    if (setcc)
                        emit_test_x86reg_x86reg(EAX, EAX);
                }
            } else if (op == 8) { // TST
                if (right_is_imm) {
                    emit_test_armreg_immediate(left_reg, imm);
                } else {
                    if (right_is_reg)
                        emit_mov_x86reg_armreg(EAX, right_reg);
                    emit_test_armreg_x86reg(left_reg, EAX);
                }
            } else if (op == 10) { // CMP
                if (right_is_imm) {
                    emit_alu_armreg_immediate(CMP, left_reg, imm);
                } else {
                    if (right_is_reg)
                        emit_mov_x86reg_armreg(EAX, right_reg);
                    emit_alu_armreg_x86reg(CMP, left_reg, EAX);
                }
                set_overflow = SETO;
                set_carry = SETAE;
            } else if (op == 9 || op == 11) { // TEQ, CMN
                int aluop;
                if (op == 9) { aluop = XOR; }
                else         { aluop = ADD; set_overflow = SETO; set_carry = SETB; }

                if (right_is_imm) {
                    emit_mov_x86reg_armreg(EAX, left_reg);
                    emit_alu_x86reg_immediate(aluop, EAX, imm);
                } else {
                    if (right_is_reg)
                        emit_mov_x86reg_armreg(EAX, right_reg);
                    emit_alu_x86reg_armreg(aluop, EAX, left_reg);
                }
            } else {
                int aluop;
                enum { LR = 1, RL = 2 } direction;

                if      (op == 0)  { aluop = AND; direction = LR | RL; }
                else if (op == 1)  { aluop = XOR; direction = LR | RL; }
                else if (op == 2)  { aluop = SUB; direction = LR;      set_overflow = SETO; set_carry = SETAE; }
                else if (op == 3)  { aluop = SUB; direction = RL;      set_overflow = SETO; set_carry = SETAE; }
                else if (op == 4)  { aluop = ADD; direction = LR | RL; set_overflow = SETO; set_carry = SETB; }
                else if (op == 5)  { aluop = ADC; direction = LR | RL; set_overflow = SETO; set_carry = SETB; }
                else if (op == 6)  { aluop = SBB; direction = LR;      set_overflow = SETO; set_carry = SETAE; }
                else if (op == 7)  { aluop = SBB; direction = RL;      set_overflow = SETO; set_carry = SETAE; }
                else if (op == 12) { aluop = OR;  direction = LR | RL; }
                else {
                    // Convert BIC to AND
                    if (right_is_imm) {
                        imm = ~imm;
                    } else {
                        if (right_is_reg) {
                            emit_mov_x86reg_armreg(EAX, right_reg);
                            right_is_reg = 0;
                        }
                        emit_unary_x86reg(NOT, EAX);
                    }
                    aluop = AND; direction = LR | RL;
                }

                if (aluop == ADC) {
                    emit_mov_x86reg8_immediate(CL, 0);
                    emit_alu_x86reg8_flag(CMP, CL, &arm.cpsr_c);
                } else if (aluop == SBB) {
                    emit_cmp_flag_immediate(&arm.cpsr_c, 1);
                }

                int reg_out = EAX;
                if (dest_reg == left_reg && (direction & LR)) {
                    if (right_is_imm) {
                        emit_alu_armreg_immediate(aluop, dest_reg, imm);
                    } else {
                        if (right_is_reg)
                            emit_mov_x86reg_armreg(EAX, right_reg);
                        emit_alu_armreg_x86reg(aluop, dest_reg, EAX);
                    }
                } else if (right_is_reg && dest_reg == right_reg && (direction & RL)) {
                    emit_mov_x86reg_armreg(EAX, left_reg);
                    emit_alu_armreg_x86reg(aluop, dest_reg, EAX);
                } else {
                    if (right_is_imm) {
                        if (direction & LR) {
                            emit_mov_x86reg_armreg(EAX, left_reg);
                            emit_alu_x86reg_immediate(aluop, EAX, imm);
                        } else {
                            if (aluop == SUB && imm == 0) {
                                if (dest_reg == left_reg) {
                                    /* RSB reg, reg, 0 is like x86's NEG */
                                    emit_unary_armreg(NEG, left_reg);
                                    goto data_proc_done;
                                }
                                emit_alu_x86reg_x86reg(XOR, EAX, EAX);
                            } else {
                                emit_mov_x86reg_immediate(EAX, imm);
                            }
                            emit_alu_x86reg_armreg(aluop, EAX, left_reg);
                        }
                    } else if (right_is_reg) {
                        if (direction & LR) {
                            emit_mov_x86reg_armreg(EAX, left_reg);
                            emit_alu_x86reg_armreg(aluop, EAX, right_reg);
                        } else {
                            emit_mov_x86reg_armreg(EAX, right_reg);
                            emit_alu_x86reg_armreg(aluop, EAX, left_reg);
                        }
                    } else {
                        if (direction & RL) {
                            emit_alu_x86reg_armreg(aluop, EAX, left_reg);
                        } else {
                            emit_mov_x86reg_armreg(REG_ARG2, left_reg);
                            emit_alu_x86reg_x86reg(aluop, REG_ARG2, EAX);
                            reg_out = REG_ARG2;
                        }
                    }
                    emit_mov_armreg_x86reg(dest_reg, reg_out);
                }
            }
data_proc_done:
            if (setcc) {
                emit_setcc_flag(SETS, &arm.cpsr_n);
                emit_setcc_flag(SETZ, &arm.cpsr_z);
                if (set_carry >= 0) {
                    if (set_carry < 2)
                        emit_mov_flag_immediate(&arm.cpsr_c, set_carry);
                    else
                        emit_setcc_flag(set_carry, &arm.cpsr_c);
                }
                if (set_overflow >= 0)
                    emit_setcc_flag(set_overflow, &arm.cpsr_v);
            }
        } else if ((insn & 0xC000000) == 0x4000000) {
            /* Byte/word memory access */
            int post_index = !(insn & (1 << 24));
            int offset_op = (insn & (1 << 23)) ? ADD : SUB;
            int is_byteop = insn & (1 << 22);
            int pre_index = insn & (1 << 21);
            int is_load   = insn & (1 << 20);
            int base_reg = insn >> 16 & 15;
            int data_reg = insn >> 12 & 15;

            if (pre_index || post_index) {
                // Pre-indexed addressing is broken (maybe data abort issues?)
                if (pre_index) break;
                if (pre_index && post_index) break;
                if (base_reg == 15) break;
                if (is_load && base_reg == data_reg) break;
            }

            if (insn & (1 << 25)) {
                // Offset is register

                int offset_reg = insn & 15;
                int shift_type = insn >> 5 & 3;
                static const uint8_t shift_table[] = { SHL, SHR, SAR, ROR };
                int count;

                if (insn & (1 << 4))
                    // reg shifted by reg
                    break;

                // reg shifted by immediate
                count = insn >> 7 & 31;
                if (count == 0 && shift_type != 0)
                    break; // special shift

                if (base_reg == 15)
                    emit_mov_x86reg_immediate(REG_ARG1, pc + 8);
                else
                    emit_mov_x86reg_armreg(REG_ARG1, base_reg);

                if (count == 0 && !pre_index && !post_index) {
                    emit_alu_x86reg_armreg(offset_op, REG_ARG1, offset_reg);
                } else {
                    emit_mov_x86reg_armreg(ECX, offset_reg);
                    emit_shift_x86reg(shift_table[shift_type], ECX, count);
                    if (!post_index)
                        emit_alu_x86reg_x86reg(offset_op, REG_ARG1, ECX);
                }
            } else {
                // Offset is immediate
                int offset = insn & 0xFFF;
                if (base_reg == 15) {
                    if (offset_op == SUB)
                        offset = -offset;
                    emit_mov_x86reg_immediate(REG_ARG1, pc + 8 + offset);
                } else {
                    emit_mov_x86reg_armreg(REG_ARG1, base_reg);
                    if (offset != 0 && !post_index)
                        emit_alu_x86reg_immediate(offset_op, REG_ARG1, offset);
                }
            }

            if (is_load) {
                /* LDR/LDRB instruction */
                emit_call(is_byteop ? (uintptr_t)read_byte : (uintptr_t)read_word);
                if (data_reg != 15)
                    emit_mov_armreg_x86reg(data_reg, EAX);
            } else {
                /* STR/STRB instruction */
                if (data_reg == 15)
                    emit_mov_x86reg_immediate(REG_ARG2, pc + 12);
                else
                    emit_mov_x86reg_armreg(REG_ARG2, data_reg);
                emit_call(is_byteop ? (uintptr_t)write_byte : (uintptr_t)write_word);
            }

            if (pre_index || post_index) { // Writeback
                if (insn & (1 << 25)) // Register offset
                    emit_alu_armreg_x86reg(offset_op, base_reg, ECX);
                else // Immediate offset
                    emit_alu_armreg_immediate(offset_op, base_reg, insn & 0xFFF);
            }

            if (is_load && data_reg == 15) {
                emit_jump((uintptr_t)translation_next_bx);
                stop_here = 1;
            }
        } else if ((insn & 0xE000000) == 0x8000000) {
            /* Load/store multiple */
            int writeback = insn & (1 << 21);
            int load      = insn & (1 << 20);
            int reg, offset, wb_offset, count;
            bool loaded_addr_reg = false;

            if (insn & (1 << 22)) // restore CPSR, or use umode regs
                goto unimpl;

            int addr_reg = insn >> 16 & 15;
            if (addr_reg == 15)
                goto unimpl;

            if (writeback && load && insn & (1 << addr_reg))
                goto unimpl;

            for (reg = count = 0; reg < 16; reg++)
                count += (insn >> reg & 1);

            if (insn & (1 << 23)) { /* Increasing */
                wb_offset = count * 4;
                offset = 0;
                if (insn & (1 << 24)) // Preincrement
                    offset += 4;
            } else { /* Decreasing */
                wb_offset = count * -4;
                offset = wb_offset;
                if (!(insn & (1 << 24))) // Postdecrement
                    offset += 4;
            }

            emit_mov_x86reg_armreg(EDX, addr_reg);
            for (reg = 0; reg < 16; reg++) {
                if (!(insn >> reg & 1))
                    continue;
                emit_byte(0x8D); // LEA
                emit_modrm_base_offset(REG_ARG1, EDX, offset);
                if (load) {
                    emit_call((uintptr_t)read_word);
                    if (reg == addr_reg && (insn & -1 << reg & 0xFFFF)) {
                        // Loading the address register, but there are still more
                        // registers to go. In case they cause a data abort, don't
                        // write to register yet; save it to ECX
                        emit_mov_x86reg_x86reg(ECX, EAX);
                        loaded_addr_reg = true;
                    } else if (reg != 15)
                        emit_mov_armreg_x86reg(reg, EAX);
                } else {
                    if (reg == 15)
                        emit_mov_x86reg_immediate(REG_ARG2, pc + 12);
                    else
                        emit_mov_x86reg_armreg(REG_ARG2, reg);
                    emit_call((uintptr_t)write_word);
                }
                offset += 4;
            }

            if (writeback)
                emit_alu_armreg_immediate(ADD, addr_reg, wb_offset);

            if (loaded_addr_reg)
                emit_mov_armreg_x86reg(addr_reg, ECX);

            if (insn & (1 << 15) && load) {
                // LDM with PC
                emit_jump((uintptr_t)translation_next_bx);
                stop_here = 1;
            }
        } else if ((insn & 0xE000000) == 0xA000000) {
            /* Branch, branch-and-link */
            if (insn & (1 << 24))
                emit_mov_armreg_immediate(14, pc + 4);
            emit_mov_x86reg_immediate(EAX, pc + 8 + ((int32_t)insn << 8 >> 6));
            emit_jump((uintptr_t)translation_next);
            stop_here = 1;
        } else {
            break;
        }

        /* Fill in the conditional jump offset */
        if (cond_jmp_offset) {
            if (out - cond_jmp_offset > 0x7F)
                goto unimpl; /* yes, this could happen (with large LDM/STM) */
            cond_jmp_offset[-1] = out - cond_jmp_offset;
        }

        RAM_FLAGS(insnp) |= (RF_CODE_TRANSLATED | next_index << RFS_TRANSLATION_INDEX);
        pc += 4;
        insnp++;
        *outj++ = insn_start;

        if (stop_here) {
            if (cond == 0x0E)
                goto branch_unconditional;
            else
                goto branch_conditional;
        }
    }
unimpl:
    out = insn_start;
    RAM_FLAGS(insnp) |= RF_CODE_NO_TRANSLATE;
branch_conditional:
    emit_mov_x86reg_immediate(EAX, pc);
    emit_jump((uintptr_t)translation_next);
branch_unconditional:

    if (pc == start_pc)
        return;

    int index = next_index++;

    //jump_table[0] is pointer to code on pc=start_ptr
    //jump_table[1] is pointer to code on pc=start_ptr+4
    translation_table[index].jump_table = (void**) jtbl_bufptr;
    translation_table[index].start_ptr  = start_insnp;
    translation_table[index].end_ptr    = insnp;

    insn_bufptr = out;
    jtbl_bufptr = outj;
}

void flush_translations() {
    int index;
    for (index = 0; index < next_index; index++) {
        uint32_t *start = translation_table[index].start_ptr;
        uint32_t *end   = translation_table[index].end_ptr;
        for (; start < end; start++)
            RAM_FLAGS(start) &= ~(RF_CODE_TRANSLATED | (-1 << RFS_TRANSLATION_INDEX));
    }
    next_index = 0;
    insn_bufptr = insn_buffer;
    jtbl_bufptr = jtbl_buffer;
}

void invalidate_translation(int index) {
    if (in_translation_rsp) {
        uint32_t flags = RAM_FLAGS(in_translation_pc_ptr);
        if ((flags & RF_CODE_TRANSLATED) && (int)(flags >> RFS_TRANSLATION_INDEX) == index)
            error("Cannot modify currently executing code block.");
    }
    flush_translations();
}

void translate_fix_pc() {
    if (!in_translation_rsp)
        return;

    uint32_t *insnp = in_translation_pc_ptr;
    void *ret_eip = in_translation_rsp[-4];
    uint32_t flags = RAM_FLAGS(insnp);
    if (!(flags & RF_CODE_TRANSLATED))
        error("Couldn't get PC for fault");
    int index = flags >> RFS_TRANSLATION_INDEX;

    for(unsigned int i = 0; ret_eip > translation_table[index].jump_table[i]; ++i)
        arm.reg[15] += 4;

    cycle_count_delta -= ((uintptr_t)translation_table[index].end_ptr - (uintptr_t)insnp) >> 2;
    in_translation_rsp = NULL;

    assert(!(arm.cpsr_low28 & 0x20));
}
