/* Declarations for cpu.c */

#ifndef _H_CPU
#define _H_CPU

#include <string.h>

struct arm_state {  // Remember to update asmcode.S if this gets rearranged
	u32 reg[16];    // Registers for current mode.

	u32 cpsr_low28; // CPSR bits 0-27
	u8  cpsr_n;     // CPSR bit 31
	u8  cpsr_z;     // CPSR bit 30
	u8  cpsr_c;     // CPSR bit 29
	u8  cpsr_v;     // CPSR bit 28

	/* CP15 registers */
	u32 control;
	u32 translation_table_base;
	u32 domain_access_control;
	u8  data_fault_status, instruction_fault_status;
	u32 fault_address;

	u32 r8_usr[5], r13_usr[2];
	u32 r8_fiq[5], r13_fiq[2], spsr_fiq;
	u32 r13_irq[2], spsr_irq;
	u32 r13_svc[2], spsr_svc;
	u32 r13_abt[2], spsr_abt;
	u32 r13_und[2], spsr_und;

	u8  interrupts;
};
extern struct arm_state arm __asm__("arm");

#define MODE_USR 0x10
#define MODE_FIQ 0x11
#define MODE_IRQ 0x12
#define MODE_SVC 0x13
#define MODE_ABT 0x17
#define MODE_UND 0x1B
#define MODE_SYS 0x1F
#define PRIVILEGED_MODE() (arm.cpsr_low28 & 3)
#define USER_MODE()       (!(arm.cpsr_low28 & 3))

#define EX_RESET          0
#define EX_UNDEFINED      1
#define EX_SWI            2
#define EX_PREFETCH_ABORT 3
#define EX_DATA_ABORT     4
#define EX_IRQ            6
#define EX_FIQ            7

#define current_instr_size (arm.cpsr_low28 & 0x20 ? 2 /* thumb */ : 4)

void cpu_int_check();
u32 __attribute__((fastcall)) get_cpsr();
void set_cpsr_full(u32 cpsr);
void __attribute__((fastcall)) set_cpsr(u32 cpsr, u32 mask);
u32 __attribute__((fastcall)) get_spsr();
void __attribute__((fastcall)) set_spsr(u32 cpsr, u32 mask);
void cpu_exception(int type);
void cpu_interpret_instruction(u32 insn);
void cpu_arm_loop();
void cpu_thumb_loop();
void *cpu_save_state(size_t *size);
void cpu_reload_state(void *state);

#endif
