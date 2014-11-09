#ifndef _H_DISASM
#define _H_DISASM

static const char reg_name[16][4] = {
	"r0", "r1", "r2",  "r3",  "r4",  "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
};

uint32_t disasm_arm_insn(uint32_t pc);
uint32_t disasm_thumb_insn(uint32_t pc);

#endif
