#include "asmcode.h"
#include "cpu.h"
#include "debug.h"
#include "emu.h"
#include "mem.h"
#include "mmu.h"

static uint32_t shift(int type, uint32_t res, uint32_t count, int setcc) {
    //TODO: Verify!
    if (count == 0) {
        /* For all types, a count of 0 does nothing and does not affect carry. */
        return res;
    }

    switch (type) {
        default: /* not used, obviously - here to shut up gcc warning */
        case 0: /* LSL */
            if (count >= 32) {
                if (setcc) arm.cpsr_c = (count == 32) ? (res & 1) : 0;
                return 0;
            }
            if (setcc) arm.cpsr_c = res >> (32 - count) & 1;
            return res << count;
        case 1: /* LSR */
            if (count >= 32) {
                if (setcc) arm.cpsr_c = (count == 32) ? (res >> 31) : 0;
                return 0;
            }
            if (setcc) arm.cpsr_c = res >> (count - 1) & 1;
            return res >> count;
        case 2: /* ASR */
            if (count >= 32) {
                count = 31;
                if (setcc) arm.cpsr_c = res >> 31;
            } else {
                if (setcc) arm.cpsr_c = res >> (count - 1) & 1;
            }
            return (int32_t)res >> count;
        case 3: /* ROR */
            count &= 31;
            res = res >> count | res << (32 - count);
            if (setcc) arm.cpsr_c = res >> 31;
            return res;
    }
}

static uint32_t get_reg(int rn) {
    if (rn == 15) error("Invalid use of R15");
    return arm.reg[rn];
}

static uint32_t get_reg_pc_thumb(int rn) {
    return arm.reg[rn] + ((rn == 15) ? 2 : 0);
}

static inline void set_reg_pc0(int rn, uint32_t value) {
    arm.reg[rn] = value;
}

/* Detect overflow after an addition or subtraction. */
#define ADD_OVERFLOW(left, right, sum) ((int32_t)(((left) ^ (sum)) & ((right) ^ (sum))) < 0)
#define SUB_OVERFLOW(left, right, sum) ((int32_t)(((left) ^ (right)) & ((left) ^ (sum))) < 0)

/* Do an addition, setting C/V flags accordingly. */
static uint32_t add(uint32_t left, uint32_t right, int carry, int setcc) {
    uint32_t sum = left + right + carry;
    if (setcc) {
        if (sum < left) carry = 1;
        if (sum > left) carry = 0;
        arm.cpsr_c = carry;
        arm.cpsr_v = ADD_OVERFLOW(left, right, sum);
    }
    return sum;
}

static inline void set_nz_flags(uint32_t value) {
    arm.cpsr_n = value >> 31;
    arm.cpsr_z = value == 0;
}

void cpu_thumb_loop() {
    while (!exiting && arm.cycle_count_delta < 0) {
        uint16_t *insnp = (uint16_t*) read_instruction(arm.reg[15] & ~1);
        uint16_t insn = *insnp;
        uintptr_t flags = RAM_FLAGS((uintptr_t)insnp & ~3);

        if (arm.cpu_events != 0) {
            if (arm.cpu_events & ~EVENT_DEBUG_STEP)
                break;
            goto enter_debugger;
        }

        if (flags & (RF_EXEC_BREAKPOINT | RF_EXEC_DEBUG_NEXT)) {
            if (flags & RF_EXEC_BREAKPOINT)
                printf("Hit breakpoint at %08X. Entering debugger.\n", arm.reg[15]);
enter_debugger:
            debugger(DBG_EXEC_BREAKPOINT, 0);
        }

        arm.reg[15] += 2;
        arm.cycle_count_delta++;

#define CASE_x2(base) case base: case base+1
#define CASE_x4(base) CASE_x2(base): CASE_x2(base+2)
#define CASE_x8(base) CASE_x4(base): CASE_x4(base+4)
#define REG0 arm.reg[insn & 7]
#define REG3 arm.reg[insn >> 3 & 7]
#define REG6 arm.reg[insn >> 6 & 7]
#define REG8 arm.reg[insn >> 8 & 7]
        switch (insn >> 8) {
            CASE_x8(0x00): /* LSL Rd, Rm, #imm */
                CASE_x8(0x08): /* LSR Rd, Rm, #imm */
              CASE_x8(0x10): /* ASR Rd, Rm, #imm */
              set_nz_flags(REG0 = shift(insn >> 11, REG3, insn >> 6 & 31, true));
            break;
            CASE_x2(0x18): /* ADD Rd, Rn, Rm */ set_nz_flags(REG0 = add(REG3, REG6, 0, true)); break;
            CASE_x2(0x1A): /* SUB Rd, Rn, Rm */ set_nz_flags(REG0 = add(REG3, ~REG6, 1, true)); break;
            CASE_x2(0x1C): /* ADD Rd, Rn, #imm */ set_nz_flags(REG0 = add(REG3, insn >> 6 & 7, 0, true)); break;
            CASE_x2(0x1E): /* SUB Rd, Rn, #imm */ set_nz_flags(REG0 = add(REG3, ~(insn >> 6 & 7), 1, true)); break;
            CASE_x8(0x20): /* MOV Rd, #imm */ set_nz_flags(REG8 = insn & 0xFF); break;
            CASE_x8(0x28): /* CMP Rn, #imm */ set_nz_flags(add(REG8, ~(insn & 0xFF), 1, true)); break;
            CASE_x8(0x30): /* ADD Rd, #imm */ set_nz_flags(REG8 = add(REG8, insn & 0xFF, 0, true)); break;
            CASE_x8(0x38): /* SUB Rd, #imm */ set_nz_flags(REG8 = add(REG8, ~(insn & 0xFF), 1, true)); break;
            CASE_x4(0x40): {
                uint32_t *dst = &REG0;
                uint32_t res;
                uint32_t src = REG3;
                switch (insn >> 6 & 15) {
                    default:
                    case 0x0: /* AND */ res = *dst &= src; break;
                    case 0x1: /* EOR */ res = *dst ^= src; break;
                    case 0x2: /* LSL */ res = *dst = shift(0, *dst, src & 0xFF, true); break;
                    case 0x3: /* LSR */ res = *dst = shift(1, *dst, src & 0xFF, true); break;
                    case 0x4: /* ASR */ res = *dst = shift(2, *dst, src & 0xFF, true); break;
                    case 0x5: /* ADC */ res = *dst = add(*dst, src, arm.cpsr_c, true); break;
                    case 0x6: /* SBC */ res = *dst = add(*dst, ~src, arm.cpsr_c, true); break;
                    case 0x7: /* ROR */ res = *dst = shift(3, *dst, src & 0xFF, true); break;
                    case 0x8: /* TST */ res = *dst & src; break;
                    case 0x9: /* NEG */ res = *dst = add(0, ~src, 1, true); break;
                    case 0xA: /* CMP */ res = add(*dst, ~src, 1, true); break;
                    case 0xB: /* CMN */ res = add(*dst, src, 0, true); break;
                    case 0xC: /* ORR */ res = *dst |= src; break;
                    case 0xD: /* MUL */ res = *dst *= src; break;
                    case 0xE: /* BIC */ res = *dst &= ~src; break;
                    case 0xF: /* MVN */ res = *dst = ~src; break;
                }
                set_nz_flags(res);
                break;
            }
            case 0x44: { /* ADD Rd, Rm (high registers allowed) */
                uint32_t left = (insn >> 4 & 8) | (insn & 7), right = insn >> 3 & 15;
                set_reg_pc0(left, get_reg_pc_thumb(left) + get_reg_pc_thumb(right));
                break;
            }
            case 0x45: { /* CMP Rn, Rm (high registers allowed) */
                uint32_t left = (insn >> 4 & 8) | (insn & 7), right = insn >> 3 & 15;
                set_nz_flags(add(get_reg(left), ~get_reg_pc_thumb(right), 1, true));
                break;
            }
            case 0x46: { /* MOV Rd, Rm (high registers allowed) */
                uint32_t left = (insn >> 4 & 8) | (insn & 7), right = insn >> 3 & 15;
                set_reg_pc0(left, get_reg_pc_thumb(right));
                break;
            }
            case 0x47: { /* BX/BLX Rm (high register allowed) */
                uint32_t target = get_reg_pc_thumb(insn >> 3 & 15);
                if (insn & 0x80)
                    arm.reg[14] = arm.reg[15] + 1;
                arm.reg[15] = target & ~1;
                if (!(target & 1)) {
                    arm.cpsr_low28 &= ~0x20; /* Exit THUMB mode */
                    return;
                }
                break;
            }
                CASE_x8(0x48): /* LDR reg, [PC, #imm] */ REG8 = read_word(((arm.reg[15] + 2) & -4) + ((insn & 0xFF) << 2)); break;
                CASE_x2(0x50): /* STR   Rd, [Rn, Rm] */ write_word(REG3 + REG6, REG0); break;
                CASE_x2(0x52): /* STRH  Rd, [Rn, Rm] */ write_half(REG3 + REG6, REG0); break;
                CASE_x2(0x54): /* STRB  Rd, [Rn, Rm] */ write_byte(REG3 + REG6, REG0); break;
                CASE_x2(0x56): /* LDRSB Rd, [Rn, Rm] */ REG0 = (int8_t)read_byte(REG3 + REG6); break;
                CASE_x2(0x58): /* LDR   Rd, [Rn, Rm] */ REG0 = read_word(REG3 + REG6); break;
                CASE_x2(0x5A): /* LDRH  Rd, [Rn, Rm] */ REG0 = read_half(REG3 + REG6); break;
                CASE_x2(0x5C): /* LDRB  Rd, [Rn, Rm] */ REG0 = read_byte(REG3 + REG6); break;
                CASE_x2(0x5E): /* LDRSH Rd, [Rn, Rm] */ REG0 = (int16_t)read_half(REG3 + REG6); break;
                CASE_x8(0x60): /* STR  Rd, [Rn, #imm] */ write_word(REG3 + (insn >> 4 & 124), REG0); break;
                CASE_x8(0x68): /* LDR  Rd, [Rn, #imm] */ REG0 = read_word(REG3 + (insn >> 4 & 124)); break;
                CASE_x8(0x70): /* STRB Rd, [Rn, #imm] */ write_byte(REG3 + (insn >> 6 & 31), REG0); break;
                CASE_x8(0x78): /* LDRB Rd, [Rn, #imm] */ REG0 = read_byte(REG3 + (insn >> 6 & 31)); break;
                CASE_x8(0x80): /* STRH Rd, [Rn, #imm] */ write_half(REG3 + (insn >> 5 & 62), REG0); break;
                CASE_x8(0x88): /* LDRH Rd, [Rn, #imm] */ REG0 = read_half(REG3 + (insn >> 5 & 62)); break;
                CASE_x8(0x90): /* STR Rd, [SP, #imm] */ write_word(arm.reg[13] + ((insn & 0xFF) << 2), REG8); break;
                CASE_x8(0x98): /* LDR Rd, [SP, #imm] */ REG8 = read_word(arm.reg[13] + ((insn & 0xFF) << 2)); break;
                CASE_x8(0xA0): /* ADD Rd, PC, #imm */ REG8 = ((arm.reg[15] + 2) & -4) + ((insn & 0xFF) << 2); break;
                CASE_x8(0xA8): /* ADD Rd, SP, #imm */ REG8 = arm.reg[13] + ((insn & 0xFF) << 2); break;
            case 0xB0: /* ADD/SUB SP, #imm */
                arm.reg[13] += ((insn & 0x80) ? -(insn & 0x7F) : (insn & 0x7F)) << 2;
                break;

                CASE_x2(0xB4): { /* PUSH {reglist[,LR]} */
                    int i;
                    uint32_t addr = arm.reg[13];
                    for (i = 8; i >= 0; i--)
                        addr -= (insn >> i & 1) * 4;
                    uint32_t sp = addr;
                    for (i = 0; i < 8; i++)
                        if (insn >> i & 1)
                            write_word(addr, arm.reg[i]), addr += 4;
                    if (insn & 0x100)
                        write_word(addr, arm.reg[14]);
                    arm.reg[13] = sp;
                    break;
                }

                CASE_x2(0xBC): { /* POP {reglist[,PC]} */
                    int i;
                    uint32_t addr = arm.reg[13];
                    for (i = 0; i < 8; i++)
                        if (insn >> i & 1)
                            arm.reg[i] = read_word(addr), addr += 4;
                    if (insn & 0x100) {
                        uint32_t target = read_word(addr); addr += 4;
                        arm.reg[15] = target & ~1;
                        if (!(target & 1)) {
                            arm.cpsr_low28 &= ~0x20;
                            arm.reg[13] = addr;
                            return;
                        }
                    }
                    arm.reg[13] = addr;
                    break;
                }
            case 0xBE:
                printf("Software breakpoint at %08x (%02x)\n", arm.reg[15], insn & 0xFF);
                debugger(DBG_EXEC_BREAKPOINT, 0);
                break;

                CASE_x8(0xC0): { /* STMIA Rn!, {reglist} */
                    int i;
                    uint32_t addr = REG8;
                    for (i = 0; i < 8; i++)
                        if (insn >> i & 1)
                            write_word(addr, arm.reg[i]), addr += 4;
                    REG8 = addr;
                    break;
                }
                CASE_x8(0xC8): { /* LDMIA Rn!, {reglist} */
                    int i;
                    uint32_t addr = REG8;
                    uint32_t tmp = 0; // value not used, just suppressing uninitialized variable warning
                    for (i = 0; i < 8; i++) {
                        if (insn >> i & 1) {
                            if (i == (insn >> 8 & 7))
                                tmp = read_word(addr);
                            else
                                arm.reg[i] = read_word(addr);
                            addr += 4;
                        }
                    }
                    // must set address register last so it is unchanged on exception
                    REG8 = addr;
                    if (insn >> (insn >> 8 & 7) & 1)
                        REG8 = tmp;
                    break;
                }
#define BRANCH_IF(cond) if (cond) arm.reg[15] += 2 + ((int8_t)insn << 1); break;
            case 0xD0: /* BEQ */ BRANCH_IF(arm.cpsr_z)
                    case 0xD1: /* BNE */ BRANCH_IF(!arm.cpsr_z)
              case 0xD2: /* BCS */ BRANCH_IF(arm.cpsr_c)
              case 0xD3: /* BCC */ BRANCH_IF(!arm.cpsr_c)
              case 0xD4: /* BMI */ BRANCH_IF(arm.cpsr_n)
              case 0xD5: /* BPL */ BRANCH_IF(!arm.cpsr_n)
              case 0xD6: /* BVS */ BRANCH_IF(arm.cpsr_v)
              case 0xD7: /* BVC */ BRANCH_IF(!arm.cpsr_v)
              case 0xD8: /* BHI */ BRANCH_IF(arm.cpsr_c > arm.cpsr_z)
              case 0xD9: /* BLS */ BRANCH_IF(arm.cpsr_c <= arm.cpsr_z)
              case 0xDA: /* BGE */ BRANCH_IF(arm.cpsr_n == arm.cpsr_v)
              case 0xDB: /* BLT */ BRANCH_IF(arm.cpsr_n != arm.cpsr_v)
              case 0xDC: /* BGT */ BRANCH_IF(!arm.cpsr_z && arm.cpsr_n == arm.cpsr_v)
              case 0xDD: /* BLE */ BRANCH_IF(arm.cpsr_z || arm.cpsr_n != arm.cpsr_v)

              case 0xDF: /* SWI */
                  cpu_exception(EX_SWI);
                return; /* Exits THUMB mode */

                CASE_x8(0xE0): /* B */ arm.reg[15] += 2 + ((int32_t)insn << 21 >> 20); break;
                CASE_x8(0xE8): { /* Second half of BLX */
                    uint32_t target = (arm.reg[14] + ((insn & 0x7FF) << 1)) & ~3;
                    arm.reg[14] = arm.reg[15] + 1;
                    arm.reg[15] = target;
                    arm.cpsr_low28 &= ~0x20; /* Exit THUMB mode */
                    return;
                }
                CASE_x8(0xF0): /* First half of BL/BLX */
                    arm.reg[14] = arm.reg[15] + 2 + ((int32_t)insn << 21 >> 9);
                break;
                CASE_x8(0xF8): { /* Second half of BL */
                    uint32_t target = arm.reg[14] + ((insn & 0x7FF) << 1);
                    arm.reg[14] = arm.reg[15] + 1;
                    arm.reg[15] = target;
                    break;
                }
            default:
                undefined_instruction();
                break;
        }
    }
}
