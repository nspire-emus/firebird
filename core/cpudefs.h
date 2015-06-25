#ifndef CPUDEFS_H
#define CPUDEFS_H

#include "bitfield.h"

// Structure of instructions, enums etc.

union Instruction {
    uint32_t raw;

    BitField<28, 4> cond;
    BitField<0, 28> rest;

    union {
        BitField<24> l;
        BitField<0, 24> immed;
    } branch;

    union {
        BitField<0, 4> rm;
        BitField<5> l;
    } bx;

    union {
        BitField<21, 4> op;
        BitField<20> s;
        BitField<16, 4> rn;
        BitField<12, 4> rd;
        BitField<25> imm;

        // If imm
        BitField<0, 8> immed_8;
        BitField<8, 4> rotate_imm;
        // else
        BitField<4> reg_shift;
        BitField<0, 4> rm;
        BitField<5, 2> shift;
        // If reg_shift
        BitField<8, 4> rs;
        // else
        BitField<7, 5> shift_imm;
    } data_proc; // ADD, MOV, etc.

    union {
        BitField<25> not_imm;
        BitField<24> p;
        BitField<23> u;
        BitField<22> b;
        BitField<21> w;
        BitField<20> l;
        BitField<16, 4> rn;
        BitField<12, 4> rd;

        // If not_imm == 0
        BitField<0, 12> immed;
        // else
        BitField<0, 4> rm;
        BitField<5, 2> shift;
        BitField<7, 5> shift_imm;
    } mem_proc; // LDR, STRB, etc.

    union {
        BitField<24> p;
        BitField<23> u;
        BitField<22> i;
        BitField<21> w;
        BitField<20> l;
        BitField<16, 4> rn;
        BitField<12, 4> rd;

        BitField<6> s;
        BitField<5> h;

        // If i
        BitField<8, 4> immed_h;
        BitField<0, 4> immed_l;
        // else
        BitField<0, 4> rm;
    } mem_proc2; // LDRH, STRSH, etc.

    union {
        BitField<23> l;
        BitField<21> a;
        BitField<20> s;

        BitField<8, 4> rs;
        BitField<0, 4> rm;

        // If l
        BitField<16, 4> rdhi;
        BitField<12, 4> rdlo;
        // else
        BitField<16, 4> rd;
        BitField<12, 4> rn;
    } mult;

    union {
        BitField<22> r;
        BitField<12, 4> rd;
    } mrs;

    union {
        BitField<24> p;
        BitField<23> u;
        BitField<22> s;
        BitField<21> w;
        BitField<20> l;
        BitField<16, 4> rn;
    } mem_multi;
};

enum Condition {
    CC_EQ=0, CC_NE,
    CC_CS, CC_CC,
    CC_MI, CC_PL,
    CC_VS, CC_VC,
    CC_HI, CC_LS,
    CC_GE, CC_LT,
    CC_GT, CC_LE,
    CC_AL, CC_NV
};

enum ShiftType {
    SH_LSL=0,
    SH_LSR,
    SH_ASR,
    SH_ROR // RRX if count == 0
};

enum DataOp {
    OP_AND=0,
    OP_EOR,
    OP_SUB,
    OP_RSB,
    OP_ADD,
    OP_ADC,
    OP_SBC,
    OP_RSC,
    OP_TST,
    OP_TEQ,
    OP_CMP,
    OP_CMN,
    OP_ORR,
    OP_MOV,
    OP_BIC,
    OP_MVN
};

// Defined in arm_interpreter.cpp
void do_arm_instruction(Instruction i);
// Defined in coproc.cpp
void do_cp15_instruction(Instruction i);

#endif // CPUDEFS_H

