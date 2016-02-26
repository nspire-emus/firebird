#include <cassert>

#include "asmcode.h"
#include "cpu.h"
#include "cpudefs.h"
#include "debug.h"
#include "mmu.h"

// Detect overflow after an addition or subtraction
#define ADD_OVERFLOW(left, right, sum) ((int32_t)(((left) ^ (sum)) & ((right) ^ (sum))) < 0)
#define SUB_OVERFLOW(left, right, sum) ((int32_t)(((left) ^ (right)) & ((left) ^ (sum))) < 0)

static uint32_t add(uint32_t left, uint32_t right, int carry, int setcc) {
    uint64_t sum64 = left + right + carry;
    uint32_t sum = sum64;
    if(!setcc)
        return sum;

    arm.cpsr_c = sum == left ? carry : sum < left;
    arm.cpsr_v = sum64 != sum;
    return sum;
}

// "uint8_t shift_val" is correct here. If it is a shift by register, only the bottom 8 bits are looked at.
static uint32_t shift(uint32_t value, uint8_t shift_type, uint8_t shift_val, bool setcc, bool has_rs)
{
    if(shift_val == 0)
    {
        if(unlikely(!has_rs))
        {
            switch(shift_type)
            {
            case SH_ROR:
            {
                // RRX
                bool carry = arm.cpsr_c;
                if(setcc) arm.cpsr_c = value & 1;
                return value >> 1 | carry << 31;
            }
            case SH_ASR:
            case SH_LSR: // #32 is encoded as LSR #0
                return shift(value, shift_type, 32, setcc, false);
            }
        }

        return value;
    }
    else if(likely(shift_val < 32))
    {
        switch(shift_type)
        {
        case SH_LSL:
            if(setcc) arm.cpsr_c = (value >> (32 - shift_val)) & 1;
            return value << shift_val;
        case SH_LSR:
            if(setcc) arm.cpsr_c = (value >> (shift_val - 1)) & 1;
            return value >> shift_val;
        case SH_ASR:
            if(setcc) arm.cpsr_c = (value >> (shift_val - 1)) & 1;
            if(value & (1 << 31)) //TODO: Verify!
                return ~((~value) >> shift_val);
            else
                return value >> shift_val;
        case SH_ROR:
            if(setcc) arm.cpsr_c = (value >> (shift_val - 1)) & 1;
             return value >> shift_val | (value << (32 - shift_val));
        }
    }
    else if(shift_val == 32 || shift_type == SH_ASR || shift_type == SH_ROR)
    {
        switch(shift_type)
        {
        case SH_LSL: if(setcc) arm.cpsr_c = value & 1; return 0;
        case SH_LSR: if(setcc) arm.cpsr_c = !!(value & (1 << 31)); return 0;
        case SH_ASR: if(setcc) arm.cpsr_c = !!(value & (1 << 31));
            if(value & (1 << 31))
                return 0xFFFFFFFF;
            else
                return 0x00000000;
        case SH_ROR: return shift(value, SH_ROR, shift_val & 0b11111, setcc, false);
        }
    }
    else // shift_val > 32
    {
        if(setcc)
            arm.cpsr_c = 0;
        return 0;
    }

    return 0;
}

static uint32_t addr_mode_2(Instruction i)
{
    if(!i.mem_proc.not_imm)
        return i.mem_proc.immed;

    return shift(reg_pc(i.mem_proc.rm), i.mem_proc.shift, i.mem_proc.shift_imm, false, false);
}

static uint32_t rotated_imm(Instruction i, bool setcc)
{
    uint32_t imm = i.data_proc.immed_8;
    uint8_t count = i.data_proc.rotate_imm << 1;
    if(count == 0)
        return imm;

    imm = (imm >> count) | (imm << (32 - count));
    if(setcc)
        arm.cpsr_c = imm & (1 << 31);
    return imm;
}

static uint32_t addr_mode_1(Instruction i, bool setcc)
{
    if(i.data_proc.imm)
        return rotated_imm(i, setcc);

    if(i.data_proc.reg_shift)
        return shift(reg_pc(i.data_proc.rm), i.data_proc.shift, reg(i.data_proc.rs), setcc, true);
    else
        return shift(reg_pc(i.data_proc.rm), i.data_proc.shift, i.data_proc.shift_imm, setcc, false);
}

static inline void set_nz_flags(uint32_t value) {
    arm.cpsr_n = value >> 31;
    arm.cpsr_z = value == 0;
}

static inline void set_nz_flags_64(uint64_t value) {
    arm.cpsr_n = value >> 63;
    arm.cpsr_z = value == 0;
}

void do_arm_instruction(Instruction i)
{
    bool exec = true;

    // Shortcut for unconditional instructions
    if(likely(i.cond == CC_AL))
        goto always;

    switch(i.cond)
    {
    case CC_EQ: case CC_NE: exec = arm.cpsr_z; break;
    case CC_CS: case CC_CC: exec = arm.cpsr_c; break;
    case CC_MI: case CC_PL: exec = arm.cpsr_n; break;
    case CC_VS: case CC_VC: exec = arm.cpsr_v; break;
    case CC_HI: case CC_LS: exec = !arm.cpsr_z && arm.cpsr_c; break;
    case CC_GE: case CC_LT: exec = arm.cpsr_n == arm.cpsr_v; break;
    case CC_GT: case CC_LE: exec = !arm.cpsr_z && arm.cpsr_n == arm.cpsr_v; break;
    case CC_NV:
        if((i.raw & 0xFD70F000) == 0xF550F000)
            return;
        else if((i.raw & 0xFE000000) == 0xFA000000)
        {
            // BLX
            arm.reg[14] = arm.reg[15];
            arm.reg[15] += 4 + ((int32_t)i.raw << 8 >> 6) + (i.raw >> 23 & 2);
            arm.cpsr_low28 |= 0x20; // Enter Thumb mode
            return;
        }
        else
            undefined_instruction();

        return;
    }

    exec ^= i.cond & 1;

    if(!exec)
        return;

    always:
    uint32_t insn = i.raw;

    if((insn & 0xE000090) == 0x0000090)
    {
        // MUL, SWP, etc.
        // LDRH, STRSH, etc.
        int type = insn >> 5 & 3;
        if (type == 0) {
            if ((insn & 0xFC000F0) == 0x0000090) {
                /* MUL, MLA: 32x32 to 32 multiplications */
                uint32_t res = reg(insn & 15)
                        * reg(insn >> 8 & 15);
                if (insn & 0x0200000)
                    res += reg(insn >> 12 & 15);

                set_reg(insn >> 16 & 15, res);
                if (insn & 0x0100000) set_nz_flags(res);
            } else if ((insn & 0xF8000F0) == 0x0800090) {
                /* UMULL, UMLAL, SMULL, SMLAL: 32x32 to 64 multiplications */
                uint32_t left   = reg(insn & 15);
                uint32_t right  = reg(insn >> 8 & 15);
                uint32_t reg_lo = insn >> 12 & 15;
                uint32_t reg_hi = insn >> 16 & 15;

                if (reg_lo == reg_hi)
                    error("RdLo and RdHi cannot be same for 64-bit multiply");

                uint64_t res;
                if (insn & 0x0400000) res = (int64_t)(int32_t)left * (int32_t)right;
                else                  res = (uint64_t)left * right;
                if (insn & 0x0200000) {
                    /* Accumulate */
                    res += (uint64_t)reg(reg_hi) << 32 | reg(reg_lo);
                }

                set_reg(reg_lo, res);
                set_reg(reg_hi, res >> 32);
                if (insn & 0x0100000) set_nz_flags_64(res);
            } else if ((insn & 0xFB00FF0) == 0x1000090) {
                /* SWP, SWPB */
                uint32_t addr = reg(insn >> 16 & 15);
                uint32_t ld, st = reg(insn & 15);
                if (insn & 0x0400000) {
                    ld = read_byte(addr); write_byte(addr, st);
                } else {
                    ld = read_word(addr); write_word(addr, st);
                }
                set_reg(insn >> 12 & 15, ld);
            } else {
                assert(false);
            }
        } else {
            /* Load/store halfword, signed byte/halfword, or doubleword */
            int base_reg = insn >> 16 & 15;
            int data_reg = insn >> 12 & 15;
            int offset = (insn & (1 << 22))
                    ? (insn & 0x0F) | (insn >> 4 & 0xF0)
                    : reg(insn & 15);
            bool writeback = false;
            uint32_t addr = reg_pc(base_reg);

            if (!(insn & (1 << 23))) // Subtracted offset
                offset = -offset;

            if (insn & (1 << 24)) { // Offset or pre-indexed addressing
                addr += offset;
                offset = 0;
                writeback = insn & (1 << 21);
            } else {
                if(insn & (1 << 21))
                    mmu_check_priv(addr, !((insn & (1 << 20)) || type == 2));

                writeback = true;
            }

            if (insn & (1 << 20)) {
                uint32_t data;
                if (base_reg == data_reg && writeback)
                    error("Load instruction modifies base register twice");
                if      (type == 1) data =      read_half(addr); /* LDRH  */
                else if (type == 2) data = (int8_t) read_byte(addr); /* LDRSB */
                else                data = (int16_t)read_half(addr); /* LDRSH */
                set_reg(data_reg, data);
            } else if (type == 1) { /* STRH */
                write_half(addr, reg(data_reg));
            } else {
                if (data_reg & 1) error("LDRD/STRD with odd-numbered data register");
                if (type == 2) { /* LDRD */
                    if ((base_reg & ~1) == data_reg && writeback)
                        error("Load instruction modifies base register twice");
                    uint32_t low  = read_word(addr);
                    uint32_t high = read_word(addr + 4);
                    set_reg(data_reg,     low);
                    set_reg(data_reg + 1, high);
                } else { /* STRD */
                    write_word(addr,     reg(data_reg));
                    write_word(addr + 4, reg(data_reg + 1));
                }
            }
            if (writeback)
                set_reg(base_reg, addr + offset);
        }
    }
    else if((insn & 0xD900000) == 0x1000000)
    {
        // BLX, MRS, MSR, SMUL, etc.
        if ((insn & 0xFFFFFD0) == 0x12FFF10) {
            /* B(L)X: Branch(, link,) and exchange T bit */
            uint32_t target = reg_pc(insn & 15);
            if (insn & 0x20)
                arm.reg[14] = arm.reg[15];
            set_reg_bx(15, target);
        } else if ((insn & 0xFBF0FFF) == 0x10F0000) {
            /* MRS: Move reg <- status */
            set_reg(insn >> 12 & 15, (insn & 0x0400000) ? get_spsr() : get_cpsr());
        } else if ((insn & 0xFB0FFF0) == 0x120F000 ||
                   (insn & 0xFB0F000) == 0x320F000) {
            /* MSR: Move status <- reg/imm */
            uint32_t val, mask = 0;
            if (insn & 0x2000000)
                val = rotated_imm(i, false);
            else
                val = reg(insn & 15);
            if (insn & 0x0080000) mask |= 0xFF000000;
            if (insn & 0x0040000) mask |= 0x00FF0000;
            if (insn & 0x0020000) mask |= 0x0000FF00;
            if (insn & 0x0010000) mask |= 0x000000FF;
            if (insn & 0x0400000)
                set_spsr(val, mask);
            else
                set_cpsr(val, mask);
        } else if ((insn & 0xF900090) == 0x1000080) {
            int32_t left = reg(insn & 15);
            int16_t right = reg((insn >> 8) & 15) >> ((insn & 0x40) ? 16 : 0);
            int32_t product;
            int type = insn >> 21 & 3;

            if (type == 1) {
                /* SMULW<y>, SMLAW<y>: Signed 32x16 to 48 multiply, uses only top 32 bits */
                product = (int64_t)left * right >> 16;
                if (!(insn & 0x20))
                    goto accumulate;
            } else {
                /* SMUL<x><y>, SMLA<x><y>, SMLAL<x><y>: Signed 16x16 to 32 multiply */
                product = (int16_t)(left >> ((insn & 0x20) ? 16 : 0)) * right;
            }
            if (type == 2) {
                /* SMLAL<x><y>: 64-bit accumulate */
                uint32_t reg_lo = insn >> 12 & 15;
                uint32_t reg_hi = insn >> 16 & 15;
                int64_t sum;
                if (reg_lo == reg_hi)
                    error("RdLo and RdHi cannot be same for 64-bit accumulate");
                sum = product + ((uint64_t)reg(reg_hi) << 32 | reg(reg_lo));
                set_reg(reg_lo, sum);
                set_reg(reg_hi, sum >> 32);
            } else if (type == 0) accumulate: {
                /* SMLA<x><y>, SMLAW<y>: 32-bit accumulate */
                int32_t acc = reg(insn >> 12 & 15);
                int32_t sum = product + acc;
                /* Set Q flag on overflow */
                arm.cpsr_low28 |= ADD_OVERFLOW(product, acc, sum) << 27;
                set_reg(insn >> 16 & 15, sum);
            } else {
                /* SMUL<x><y>, SMULW<y>: No accumulate */
                set_reg(insn >> 16 & 15, product);
            }
        } else if ((insn & 0xF900FF0) == 0x1000050) {
            /* QADD, QSUB, QDADD, QDSUB: Saturated arithmetic */
            int32_t left  = reg(insn       & 15);
            int32_t right = reg(insn >> 16 & 15);
            int32_t res, overflow;
            if (insn & 0x400000) {
                /* Doubled right operand */
                res = right << 1;
                if (ADD_OVERFLOW(right, right, res)) {
                    /* Overflow, set Q flag and saturate */
                    arm.cpsr_low28 |= 1 << 27;
                    res = (res < 0) ? 0x7FFFFFFF : 0x80000000;
                }
                right = res;
            }
            if (!(insn & 0x200000)) {
                res = left + right;
                overflow = ADD_OVERFLOW(left, right, res);
            } else {
                res = left - right;
                overflow = SUB_OVERFLOW(left, right, res);
            }
            if (overflow) {
                /* Set Q flag and saturate */
                arm.cpsr_low28 |= 1 << 27;
                res = (res < 0) ? 0x7FFFFFFF : 0x80000000;
            }
            set_reg(insn >> 12 & 15, res);
        } else if ((insn & 0xFFF0FF0) == 0x16F0F10) {
            /* CLZ: Count leading zeros */
            int32_t value = reg(insn & 15);
            uint32_t zeros;
            for (zeros = 0; zeros < 32 && value >= 0; zeros++)
                value <<= 1;
            set_reg(insn >> 12 & 15, zeros);
        } else if ((insn & 0xFFF000F0) == 0xE1200070) {
            gui_debug_printf("Software breakpoint at %08x (%04x)\n",
                      arm.reg[15], (insn >> 4 & 0xFFF0) | (insn & 0xF));
            debugger(DBG_EXEC_BREAKPOINT, 0);
        } else
            undefined_instruction();
    }
    else if(likely((insn & 0xC000000) == 0x0000000))
    {
        // Data processing
        bool carry = arm.cpsr_c,
             setcc = i.data_proc.s;

        uint32_t left = reg_pc(i.data_proc.rn),
                 right = addr_mode_1(i, setcc),
                 res = 0;

        switch(i.data_proc.op)
        {
        case OP_AND: res = left & right; break;
        case OP_EOR: res = left ^ right; break;
        case OP_SUB: res = add( left, ~right, 1, setcc); break;
        case OP_RSB: res = add(~left,  right, 1, setcc); break;
        case OP_ADD: res = add( left,  right, 0, setcc); break;
        case OP_ADC: res = add( left,  right, carry, setcc); break;
        case OP_SBC: res = add( left, ~right, carry, setcc); break;
        case OP_RSC: res = add(~left,  right, carry, setcc); break;
        case OP_TST: res = left & right; break;
        case OP_TEQ: res = left ^ right; break;
        case OP_CMP: res = add( left, ~right, 1, setcc); break;
        case OP_CMN: res = add( left,  right, 0, setcc); break;
        case OP_ORR: res = left | right; break;
        case OP_MOV: res = right; break;
        case OP_BIC: res = left & ~right; break;
        case OP_MVN: res = ~right; break;
        }

        if(i.data_proc.op < OP_TST || i.data_proc.op > OP_CMN)
            set_reg_pc(i.data_proc.rd, res);

        if(setcc)
        {
            // Used for returning from exceptions, for instance
            if(i.data_proc.rd == 15)
                set_cpsr_full(get_spsr());
            else
            {
                arm.cpsr_n = res >> 31;
                arm.cpsr_z = res == 0;
            }
        }
    }
    else if((insn & 0xFF000F0) == 0x7F000F0)
        undefined_instruction();
    else if((insn & 0xC000000) == 0x4000000)
    {
        // LDR, STRB, etc.
        uint32_t base = reg_pc(i.mem_proc.rn),
                 offset = addr_mode_2(i);
        if(!i.mem_proc.u)
            offset = -offset;

        // Pre-indexed or offset
        if(i.mem_proc.p)
            base += offset; // Writeback for pre-indexed handled after access
        else if(i.mem_proc.w) // Usermode Access
            mmu_check_priv(base, !i.mem_proc.l);

        // Byte access
        if(i.mem_proc.b)
        {
            if(i.mem_proc.l) set_reg_bx(i.mem_proc.rd, read_byte(base));
            else write_byte(base, reg_pc_mem(i.mem_proc.rd));
        }
        else
        {
            if(i.mem_proc.l) set_reg_bx(i.mem_proc.rd, read_word(base));
            else write_word(base, reg_pc_mem(i.mem_proc.rd));
        }

        // Post-indexed addressing
        if(!i.mem_proc.p)
            base += offset;

        // Writeback
        if(!i.mem_proc.p || i.mem_proc.w)
            set_reg(i.mem_proc.rn, base);
    }
    else if((insn & 0xE000000) == 0x8000000)
    {
        // LDM, STM, etc.
        int base_reg = insn >> 16 & 15;
        uint32_t addr = reg(base_reg);
        uint32_t new_base = addr;
        int i, count = 0;
        uint16_t reglist = insn;
        while(reglist)
        {
            count += reglist & 1;
            reglist >>= 1;
        }

        if (insn & (1 << 23)) { // Increasing
            if (insn & (1 << 21)) // Writeback
                new_base += count * 4;
            if (insn & (1 << 24)) // Preincrement
                addr += 4;
        } else { // Decreasing
            addr -= count * 4;
            if (insn & (1 << 21)) // Writeback
                new_base = addr;
            if (!(insn & (1 << 24))) // Postdecrement
                addr += 4;
        }

        for (i = 0; i < 15; i++) {
            if (insn >> i & 1) {
                uint32_t *reg_ptr = &arm.reg[i];
                if (insn & (1 << 22) && ~insn & ((1 << 20) | (1 << 15))) {
                    // User-mode registers
                    int mode = arm.cpsr_low28 & 0x1F;
                    if (i >= 13) {
                        if (mode != MODE_USR && mode != MODE_SYS) reg_ptr = &arm.r13_usr[i - 13];
                    } else if (i >= 8) {
                        if (mode == MODE_FIQ) reg_ptr = &arm.r8_usr[i - 8];
                    }
                }
                if (insn & (1 << 20)) { // Load
                    if (reg_ptr == &arm.reg[base_reg]) {
                        if (insn & (1 << 21)) // Writeback
                            error("Load instruction modifies base register twice");
                        reg_ptr = &new_base;
                    }
                    *reg_ptr = read_word(addr);
                } else { // Store
                    write_word(addr, *reg_ptr);
                }
                addr += 4;
            }
        }
        if (insn & (1 << 15)) {
            if (insn & (1 << 20)) // Load
                set_reg_bx(15, read_word(addr));
            else // Store
                write_word(addr, reg_pc_mem(15));
        }
        arm.reg[base_reg] = new_base;
        if ((~insn & (1 << 22 | 1 << 20 | 1 << 15)) == 0)
            set_cpsr_full(get_spsr());
    }
    else if((insn & 0xE000000) == 0xA000000)
    {
        // B and BL
        if(i.branch.l)
            arm.reg[14] = arm.reg[15];
        arm.reg[15] += (int32_t) i.raw << 8 >> 6;
        arm.reg[15] += 4;
    }
    else if((insn & 0xF000F10) == 0xE000F10)
        do_cp15_instruction(i);
    else if((insn & 0xF000000) == 0xF000000)
        cpu_exception(EX_SWI);
    else
        undefined_instruction();
}
