/* Declarations for cpu.c */

#ifndef _H_CPU
#define _H_CPU

#include <string.h>

struct arm_state {  // Remember to update asmcode.S if this gets rearranged
	uint32_t reg[16];    // Registers for current mode.

	uint32_t cpsr_low28; // CPSR bits 0-27
	uint8_t  cpsr_n;     // CPSR bit 31
	uint8_t  cpsr_z;     // CPSR bit 30
	uint8_t  cpsr_c;     // CPSR bit 29
	uint8_t  cpsr_v;     // CPSR bit 28

	/* CP15 registers */
	uint32_t control;
	uint32_t translation_table_base;
	uint32_t domain_access_control;
	uint8_t  data_fault_status, instruction_fault_status;
	uint32_t fault_address;

	uint32_t r8_usr[5], r13_usr[2];
	uint32_t r8_fiq[5], r13_fiq[2], spsr_fiq;
	uint32_t r13_irq[2], spsr_irq;
	uint32_t r13_svc[2], spsr_svc;
	uint32_t r13_abt[2], spsr_abt;
	uint32_t r13_und[2], spsr_und;

	uint8_t  interrupts;
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
uint32_t __attribute__((fastcall)) get_cpsr();
void set_cpsr_full(uint32_t cpsr);
void __attribute__((fastcall)) set_cpsr(uint32_t cpsr, uint32_t mask);
uint32_t __attribute__((fastcall)) get_spsr();
void __attribute__((fastcall)) set_spsr(uint32_t cpsr, uint32_t mask);
void cpu_exception(int type);
void cpu_interpret_instruction(uint32_t insn);
void cpu_arm_loop();
void cpu_thumb_loop();
void *cpu_save_state(size_t *size);
void cpu_reload_state(void *state);

#endif
