#ifndef _H_DISASM
#define _H_DISASM

static const char reg_name[16][4] = {
	"r0", "r1", "r2",  "r3",  "r4",  "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
};

u32 disasm_arm_insn(u32 pc);
u32 disasm_thumb_insn(u32 pc);

#endif
