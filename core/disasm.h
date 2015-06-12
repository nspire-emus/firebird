#ifndef _H_DISASM
#define _H_DISASM

#ifdef __cplusplus
extern "C" {
#endif

extern const char reg_name[16][4];

uint32_t disasm_arm_insn(uint32_t pc);
uint32_t disasm_thumb_insn(uint32_t pc);

#ifdef __cplusplus
}
#endif

#endif
