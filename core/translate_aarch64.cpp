/* How the AArch64 translation works:
 * AArch64 has enough registers to keep the entire ARM CPU state available.
 * In JIT'd code, these physical registers have a fixed role:
 * x0: First parameter and return value of helper functions in asmcode_aarch64.S
 * x1: Second parameter to helper functions in asmcode_aarch64.S
 * w2-w16: arm.reg[0] - arm.reg[14]
 * x17: Used as a scratch register for keeping a copy of the virtual cpsr_nzcv
 * (x18: Platform specific, can't use this)
 * x19: Pointer to the arm_state struct
 * x21-x24: Used as temporary registers by various subroutines
 * x25: Temporary register, not touched by (read|write)_asm routines
 * x26: Pointer to addr_cache contents
 * x27-x29: Not preserved, so don't touch.
 * x30: lr
 * x31: sp
 * x18 and x30 are callee-save, so must be saved and restored properly.
 * The other registers are part of the caller-save x0-x18 area and do not need
 * to be preserved. They need to be written back into the virtual arm struct
 * before calling into compiler-generated code though.
 */
#include <cassert>
#include <cstdint>

#include "cpudefs.h"
#include "emu.h"
#include "translate.h"
#include "mem.h"
#include "mmu.h"
#include "os/os.h"

#ifndef __aarch64__
#error "I'm sorry Dave, I'm afraid I can't do that."
#endif

/* Helper functions in asmcode_aarch64.S */
extern "C" {
	extern void translation_next(uint32_t new_pc) __asm__("translation_next");
	extern void translation_next_bx(uint32_t new_pc) __asm__("translation_next_bx");
	extern void translation_jmp(void *target) __asm__("translation_jmp");
	extern void **translation_sp __asm__("translation_sp");
	extern void read_word_asm() __asm("read_word_asm");
	extern void write_word_asm() __asm("write_word_asm");
	extern void read_half_asm() __asm("read_half_asm");
	extern void write_half_asm() __asm("write_half_asm");
	extern void read_byte_asm() __asm("read_byte_asm");
	extern void write_byte_asm() __asm("write_byte_asm");
#ifdef IS_IOS_BUILD
	int sys_cache_control(int function, void *start, size_t len);
#endif
}

enum Reg : uint8_t {
	R0 = 0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, SP, LR, PC
};

enum PReg : uint8_t {
	W0 = 0, W1, W2, W3, W4, W5, W6, W7, W8, W9, W10, W11, W12, W13, W14, W15, W16, W17, W25 = 25, W26,
	X0 = W0, X1 = W1, X30 = 30, X31 = 31, WZR = 31
};

/* This function returns the physical register that contains the virtual register vreg.
   vreg must not be PC. */
static PReg mapreg(const uint8_t vreg)
{
	// See the first comment on register mapping
	assert(vreg < PC);
	return static_cast<PReg>(vreg + 2);
}

static uint32_t *translate_buffer = nullptr,
	*translate_current = nullptr;

#define MAX_TRANSLATIONS 0x40000
struct translation translation_table[MAX_TRANSLATIONS];
uint32_t *jump_table[MAX_TRANSLATIONS*2],
         **jump_table_current = jump_table;
static unsigned int next_translation_index = 0;

static void emit(const uint32_t instruction)
{
	*translate_current++ = instruction;
}

static __attribute__((unused)) void emit_brk()
{
	emit(0xd4200000); // brk #0
}

// Sets the physical register wd to imm (32bit only)
static void emit_mov_imm(const PReg wd, uint32_t imm)
{
	if((imm & 0x0000FFFF) == imm)
		emit(0x52800000 | ((imm & 0xFFFF) << 5) | wd); // movz wd, #imm, lsl #0
	else if((imm & 0xFFFF0000) == imm)
		emit(0x52a00000 | ((imm >> 16) << 5) | wd); // movz wd, #imm, lsl #16
	else
	{
		emit(0x52800000 | ((imm & 0xFFFF) << 5) | wd); // movz wd, #imm, lsl #0
		emit(0x72a00000 | ((imm >> 16) << 5) | wd); // movk wd, #imm, lsl #16
	}
}

// Sets the physical register xd to imm (64bit only)
static void emit_mov_imm64(const PReg xd, uint64_t imm)
{
	emit(0xd2800000 | ((imm & 0xFFFF) << 5) | xd); // movz wd, #imm, lsl #0
	imm >>= 16;
	emit(0xf2a00000 | ((imm & 0xFFFF) << 5) | xd); // movk wd, #imm, lsl #16
	imm >>= 16;
	emit(0xf2c00000 | ((imm & 0xFFFF) << 5) | xd); // movk wd, #imm, lsl #32
	imm >>= 16;
	emit(0xf2e00000 | ((imm & 0xFFFF) << 5) | xd); // movk wd, #imm, lsl #48
}

static void emit_mov_reg(const PReg wd, const PReg wm)
{
	emit(0x2a0003e0 | (wm << 16) | wd);
}

static __attribute__((unused)) uint32_t maybe_branch(const void *target)
{
	ptrdiff_t diff = reinterpret_cast<const uint32_t*>(target) - translate_current;
	if(diff > 0x3FFFFFF || -diff > 0x4000000)
		return 0;

	return 0x12000000 | diff;
}

// Jumps directly to target, destroys x21
static void emit_jmp(const void *target)
{
/*	uint32_t branch = maybe_branch(target);
	if(branch)
		return emit(branch);*/

	const uint64_t addr = reinterpret_cast<const uint64_t>(target);

	if(addr <= 0xFFFFFFFF)
	{
		emit(0x18000055); // ldr w21, [pc, #8]
		emit(0xd61f02a0); // br w21
		emit(addr); // Lower 32bits of target
	}
	else
	{
		emit(0x58000055); // ldr x21, [pc, #8]
		emit(0xd61f02a0); // br x21
		emit(addr & 0xFFFFFFFF); // Lower 32bits of target
		emit(addr >> 32); // Higher 32bits of target
	}
}


// Calls target, destroys x21 and x30
static void emit_call(const void *target)
{
/*	uint32_t branch = maybe_branch(target);
	if(branch)
		return emit(branch) | (1 << 31);*/

	const uint64_t addr = reinterpret_cast<const uint64_t>(target);

	if(addr <= 0xFFFFFFFF)
	{
		emit(0x18000075); // ldr x21, [pc, #8]
		emit(0x1000007e); // adr x30, <after literal)
		emit(0xd61f02a0); // br x21
		emit(addr);
	}
	else
	{
		emit(0x58000075); // ldr x21, [pc, #8]
		emit(0x1000009e); // adr x30, <after literal)
		emit(0xd61f02a0); // br x21
		emit(addr & 0xFFFFFFFF); // Lower 32bits of target
		emit(addr >> 32); // Higher 32bits of target
	}
}

bool translate_init()
{
	if(translate_buffer)
		return true;

	translate_current = translate_buffer = reinterpret_cast<uint32_t*>(os_alloc_executable(INSN_BUFFER_SIZE));
	jump_table_current = jump_table;
	next_translation_index = 0;

	return translate_buffer != nullptr;
}

void translate_deinit()
{
	if(!translate_buffer)
		return;

	os_free(translate_buffer, INSN_BUFFER_SIZE);
	translate_current = translate_buffer = nullptr;
}

void translate(uint32_t pc_start, uint32_t *insn_ptr_start)
{
	if(next_translation_index >= MAX_TRANSLATIONS)
	{
		warn("Out of translation slots!");
		return;
	}

	uint32_t **jump_table_start = jump_table_current;
	uint32_t pc = pc_start, *insn_ptr = insn_ptr_start;
	// Pointer to translation of current instruction
	uint32_t *translate_buffer_inst_start = translate_current;
	// Pointer to struct translation for this block
	translation *this_translation = &translation_table[next_translation_index];

	// Whether we can avoid the jump to translation_next at the end
	bool jumps_away = false;
	// Whether to stop translating code
	bool stop_here = false;
	// cond_branch points to the b.cond, see below
	uint32_t *cond_branch = nullptr;

	// We know this already. end_ptr will be set after the loop
	this_translation->jump_table = reinterpret_cast<void**>(jump_table_start);
	this_translation->start_ptr = insn_ptr_start;

	while(1)
	{
		// Translate further?
		if(stop_here
		   || size_t((translate_current + 16) - translate_buffer) > (INSN_BUFFER_SIZE/sizeof(*translate_buffer))
		   || RAM_FLAGS(insn_ptr) & (RF_EXEC_BREAKPOINT | RF_EXEC_DEBUG_NEXT | RF_EXEC_HACK | RF_CODE_TRANSLATED | RF_CODE_NO_TRANSLATE)
		   || (pc ^ pc_start) & ~0x3ff)
			goto exit_translation;

		Instruction i;
		i.raw = *insn_ptr;

		*jump_table_current = translate_current;

		if(unlikely(i.cond == CC_NV))
		{
			if((i.raw & 0xFD70F000) == 0xF550F000)
				goto instruction_translated; // PLD
			else
				goto unimpl;
		}
		else if(unlikely(i.cond != CC_AL))
		{
			// Conditional instruction -> Generate jump over code with inverse condition
			cond_branch = translate_current;
			emit(0x54000000 | (i.cond ^ 1));
		}

		if((i.raw & 0xE000090) == 0x0000090)
		{
			if(!i.mem_proc2.s && !i.mem_proc2.h)
			{
				// Not mem_proc2 -> multiply or swap
				if((i.raw & 0x0FB00000) == 0x01000000)
					goto unimpl; // SWP/SWPB not implemented

				// MUL, UMLAL, etc.
				if(i.mult.rm == PC || i.mult.rs == PC
				   || i.mult.rn == PC || i.mult.rd == PC)
					goto unimpl; // PC as register not implemented

				if(i.mult.s)
					goto unimpl; // setcc not implemented (not easily possible as no aarch64 instruction only changes n and z)

				if ((i.raw & 0xFC000F0) == 0x0000090)
				{
					uint32_t instruction = 0x1B000000; // madd w0, w0, w0, w0

					instruction |= (mapreg(i.mult.rs) << 16) | (mapreg(i.mult.rm) << 5) | mapreg(i.mult.rd);
					if(i.mult.a)
						instruction |= mapreg(i.mult.rn) << 10;
					else
						instruction |= WZR << 10;

					emit(instruction);
					goto instruction_translated;
				}

				goto unimpl; // UMULL, UMLAL, SMULL, SMLAL not implemented
			}

			if(i.mem_proc2.s || !i.mem_proc2.h)
				goto unimpl; // Signed byte/halfword and doubleword not implemented

			if(i.mem_proc2.rn == PC
			   || i.mem_proc2.rd == PC
			   || i.mem_proc2.rm == PC)
				goto unimpl; // PC as operand or dest. not implemented

			// Load base into w0
			emit_mov_reg(W0, mapreg(i.mem_proc2.rn));

			// Offset into w25
			if(i.mem_proc2.i) // Immediate offset
				emit_mov_imm(W25, (i.mem_proc2.immed_h << 4) | i.mem_proc2.immed_l);
			else // Register offset
				emit_mov_reg(W25, mapreg(i.mem_proc2.rm));

			// Get final address..
			if(i.mem_proc2.p)
			{
				// ..into r0
				if(i.mem_proc2.u)
					emit(0x0b190000); // add w0, w0, w25
				else
					emit(0x4b190000); // sub w0, w0, w25

				if(i.mem_proc2.w) // Writeback: final address into rn
					emit_mov_reg(mapreg(i.mem_proc2.rn), W0);
			}
			else
			{
				// ..into r5
				if(i.mem_proc2.u)
					emit(0x0b190019); // add w25, w0, w25
				else
					emit(0x4b190019); // sub w25, w0, w25
			}

			if(i.mem_proc2.l)
			{
				emit_call(reinterpret_cast<void*>(read_half_asm));
				emit_mov_reg(mapreg(i.mem_proc2.rd), W0);
			}
			else
			{
				emit_mov_reg(W1, mapreg(i.mem_proc2.rd));
				emit_call(reinterpret_cast<void*>(write_half_asm));
			}

			// Post-indexed: final address in r5 back into rn
			if(!i.mem_proc2.p)
				emit_mov_reg(mapreg(i.mem_proc2.rn), W25);

		}
		else if((i.raw & 0xD900000) == 0x1000000)
		{
			if((i.raw & 0xFFFFFD0) == 0x12FFF10)
			{
				//B(L)X
				if(i.bx.rm == PC)
					goto unimpl;

				if(i.bx.l)
					emit_mov_imm(mapreg(LR), pc + 4);
				else if(i.cond == CC_AL)
					stop_here = jumps_away = true;

				emit_mov_reg(W0, mapreg(i.bx.rm));
				emit_jmp(reinterpret_cast<void*>(translation_next_bx));
			}
			else
				goto unimpl;
		}
		else if((i.raw & 0xC000000) == 0x0000000)
		{
			// Data processing
			if(!i.data_proc.imm && i.data_proc.reg_shift) // reg shift
				goto unimpl;

			if(i.data_proc.s
			   && !(i.data_proc.op == OP_ADD || i.data_proc.op == OP_SUB || i.data_proc.op == OP_CMP || i.data_proc.op == OP_CMN
				#ifndef SUPPORT_LINUX
					|| i.data_proc.op == OP_TST
				#endif
			   ))
			{
				/* We can't translate the S-bit that easily,
				   as the barrel shifter output does not influence
				   the carry flag anymore. */
				goto unimpl;
			}

			if(i.data_proc.op == OP_RSB)
			{
				// RSB is not possible on AArch64, so try to convert it to SUB
				if(!i.data_proc.imm && !i.data_proc.reg_shift && i.data_proc.shift == SH_LSL && i.data_proc.shift_imm == 0)
				{
					// Swap rm and rn, change op to SUB
					unsigned int tmp = i.data_proc.rm;
					i.data_proc.rm = i.data_proc.rn;
					i.data_proc.rn = tmp;
					i.data_proc.op = OP_SUB;
				}
				else
					goto unimpl;
			}

			if(i.data_proc.op == OP_RSC || i.data_proc.op == OP_TEQ)
				goto unimpl;

			if(i.data_proc.shift == SH_ROR && !i.data_proc.imm && (i.data_proc.op == OP_SUB || i.data_proc.op == OP_ADD || i.data_proc.shift_imm == 0))
				goto unimpl; // ADD/SUB do not support ROR. RRX (encoded as ror #0) is not supported anywhere

			// Using pc is not supported
			if(i.data_proc.rd == PC
			   || (!i.data_proc.imm && i.data_proc.rm == PC))
				goto unimpl;

			// AArch64 ADC and SBC do not support a shifted reg
			if((i.data_proc.op == OP_ADC || i.data_proc.op == OP_SBC)
			   && (i.data_proc.shift != SH_LSL || i.data_proc.shift_imm != 0))
				goto unimpl;

			// Shortcut for simple register mov (mov rd, rm)
			if((i.raw & 0xFFF0FF0) == 0x1a00000)
			{
				emit_mov_reg(mapreg(i.data_proc.rd), mapreg(i.data_proc.rm));
				goto instruction_translated;
			}

			static const uint32_t opmap[] =
			{
				0x0A000000, // AND
				0x4A000000, // EOR
				0x4B000000, // SUB
				0, // RSB not possible
				0x0B000000, // ADD
				0x1A000000, // ADC (no shift!)
				0x5A000000, // SBC (no shift!)
				0, // RSC not possible
				#ifdef SUPPORT_LINUX
					0, // TST not possible, carry and overflow flags not identical
				#else
					0x6A00001F, // TST
				#endif
				0, // TEQ not possible
				0x6B00001F, // CMP
				0x2B00001F, // CMN
				0x2A000000, // ORR
				0x2A0003E0, // MOV (ORR with rn = wzr)
				0x0A200000, // BIC
				0x2A2003E0, // MVN (ORRN with rn = wzr)
			};

			uint32_t instruction = opmap[i.data_proc.op];

			if(i.data_proc.s)
				instruction |= 1 << 29;

			if(i.data_proc.op < OP_TST || i.data_proc.op > OP_CMP)
				instruction |= mapreg(i.data_proc.rd);

			if(i.data_proc.op != OP_MOV && i.data_proc.op != OP_MVN)
			{
				if(i.data_proc.rn != PC)
					instruction |= mapreg(i.data_proc.rn) << 5;
				else
				{
					emit_mov_imm(W1, pc + 8);
					instruction |= W1 << 5;
				}
			}

			if(!i.data_proc.imm)
			{
				instruction |= mapreg(i.data_proc.rm) << 16;
				instruction |= i.data_proc.shift << 22;
				instruction |= i.data_proc.shift_imm << 10;
			}
			else
			{
				// TODO: Could be optimized further,
				// some AArch64 ops support immediate values

				uint32_t immed = i.data_proc.immed_8;
				uint8_t count = i.data_proc.rotate_imm << 1;
				if(count)
					immed = (immed >> count) | (immed << (32 - count));

				if(i.data_proc.op == OP_MOV)
				{
					emit_mov_imm(mapreg(i.data_proc.rd), immed);
					goto instruction_translated;
				}

				if(immed <= 0xFFF
				   && (i.data_proc.op == OP_ADD || i.data_proc.op == OP_SUB
				       || i.data_proc.op == OP_CMP || i.data_proc.op == OP_CMN))
				{
					/* Those instructions support a normal 12bit (optionally shifted left by 12) immediate.
					   This is not the case for logical instructions, they use awfully complicated useless
					   terrible garbage called "bitmask operand" that not even binutils can encode properly. */
					instruction &= ~(0x1E000000);
					instruction |= 1 << 28 | (immed << 10);
				}
				else
				{
					emit_mov_imm(W0, immed);
					/* All those operations are nops (or with 0)
					instruction |= W0 << 16;
					instruction |= SH_LSL << 22;
					instruction |= 0 << 10;*/
				}
			}

			emit(instruction);
		}
		else if((i.raw & 0xFF000F0) == 0x7F000F0)
			goto unimpl; // undefined
		else if((i.raw & 0xC000000) == 0x4000000)
		{
			// Memory access: LDR, STRB, etc.
			// User mode access not implemented
			if(!i.mem_proc.p && i.mem_proc.w)
				goto unimpl;

			if(i.mem_proc.not_imm && (i.mem_proc.rm == PC || (i.mem_proc.shift == SH_ROR && i.mem_proc.shift_imm == 0)))
				goto unimpl;

			if((i.mem_proc.rd == i.mem_proc.rn || i.mem_proc.rn == PC) && (!i.mem_proc.p || i.mem_proc.w))
				goto unimpl; // Writeback into PC not implemented

			bool offset_is_zero = !i.mem_proc.not_imm && i.mem_proc.immed == 0;

			// Address into w0
			if(i.mem_proc.rn != PC)
				emit_mov_reg(W0, mapreg(i.mem_proc.rn));
			else if(i.mem_proc.not_imm)
				emit_mov_imm(W0, pc + 8);
			else // Address known
			{
				int offset = i.mem_proc.u ? i.mem_proc.immed :
				                            -i.mem_proc.immed;

				emit_mov_imm(W0, pc + 8 + offset);
				offset_is_zero = true;
			}

			// Skip offset calculation
			if(offset_is_zero)
				goto no_offset;

			// Offset into w25
			if(!i.mem_proc.not_imm) // Immediate offset
				emit_mov_imm(W25, i.mem_proc.immed);
			else // Register offset, shifted
			{
				uint32_t instruction = 0x2a0003f9; // orr w25, wzr, wX, <shift>, #<amount>
				instruction |= mapreg(i.mem_proc.rm) << 16;
				instruction |= i.mem_proc.shift << 22;
				instruction |= i.mem_proc.shift_imm << 10;

				emit(instruction);
			}

			// Get final address..
			if(i.mem_proc.p)
			{
				// ..into r0
				if(i.mem_proc.u)
					emit(0x0b190000); // add w0, w0, w25
				else
					emit(0x4b190000); // sub w0, w0, w25

				// It has to be stored AFTER the memory access, to be able
				// to perform the access again after an abort.
				if(i.mem_proc.w) // Writeback: final address into w25
					emit_mov_reg(W25, W0);
			}
			else
			{
				// ..into w25
				if(i.mem_proc.u)
					emit(0x0b190019); // add w25, w0, w25
				else
					emit(0x4b190019); // sub w25, w0, w25
			}

			no_offset:
			if(i.mem_proc.l)
			{
				emit_call(reinterpret_cast<void*>(i.mem_proc.b ? read_byte_asm : read_word_asm));
				if(i.mem_proc.rd != PC)
					emit_mov_reg(mapreg(i.mem_proc.rd), W0);
			}
			else
			{
				if(i.mem_proc.rd != PC)
					emit_mov_reg(W1, mapreg(i.mem_proc.rd)); // w1 is the value
				else
					emit_mov_imm(W1, pc + 12);

				emit_call(reinterpret_cast<void*>(i.mem_proc.b ? write_byte_asm : write_word_asm));
			}

			if(!offset_is_zero && (!i.mem_proc.p || i.mem_proc.w))
				emit_mov_reg(mapreg(i.mem_proc.rn), W25);

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
		else if((i.raw & 0xE000000) == 0x8000000)
		{
			// LDM/STM
			if(i.mem_multi.s)
				goto unimpl; // Exception return or usermode not implemented

			if(i.mem_multi.rn == PC)
				goto unimpl;

			uint16_t reglist = i.mem_multi.reglist;
			if(reglist & (1 << i.mem_multi.rn))
				goto unimpl; // Loading/Saving address register

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

			// Base reg in w25
			emit_mov_reg(W25, mapreg(i.mem_multi.rn));
			unsigned int reg = 0;
			while(reglist)
			{
				if((reglist & 1) == 0)
					goto next;

				if(offset >= 0)
					emit(0x11000320 | (offset << 10)); // add w0, w25, #offset
				else
					emit(0x51000320 | (-offset << 10)); // sub w0, w25, #-offset

				if(i.mem_multi.l)
				{
					emit_call(reinterpret_cast<void*>(read_word_asm));
					if(reg != PC)
						emit_mov_reg(mapreg(reg), W0);
					//else written below
				}
				else
				{
					if(reg == PC)
						emit_mov_imm(W1, pc + 12);
					else
						emit_mov_reg(W1, mapreg(reg));

					emit_call(reinterpret_cast<void*>(write_word_asm));
				}

				offset += 4;
				next:
				reglist >>= 1;
				++reg;
			}

			if(i.mem_multi.w)
			{
				if(wb_offset >= 0)
					emit(0x11000320 | (wb_offset << 10) | mapreg(i.mem_multi.rn)); // add wN, w25, #wb_offset
				else
					emit(0x51000320 | (-wb_offset << 10) | mapreg(i.mem_multi.rn)); // sub wN, w25, #-wb_offset
			}

			if(i.mem_multi.l && (i.mem_multi.reglist & (1 << PC))) // Loading PC
			{
				// PC already in W0 (last one loaded)
				emit_jmp(reinterpret_cast<void*>(translation_next_bx));
				if(i.cond == CC_AL)
					jumps_away = stop_here = true;
			}
		}
		else if((i.raw & 0xE000000) == 0xA000000)
		{
			if(i.branch.l)
			{
				// Save return address in LR
				emit_mov_imm(mapreg(LR), pc + 4);
			}
			else if(i.cond == CC_AL)
				jumps_away = stop_here = true;

			uint32_t addr = pc + ((int32_t)(i.raw << 8) >> 6) + 8;
			uintptr_t entry = reinterpret_cast<uintptr_t>(addr_cache[(addr >> 10) << 1]);
			uint32_t *ptr = reinterpret_cast<uint32_t*>(entry + addr);

			if(entry & AC_FLAGS || !(RAM_FLAGS(ptr) & RF_CODE_TRANSLATED))
			{
				emit_mov_imm(W0, addr);
				emit_jmp(reinterpret_cast<void*>(translation_next));
			}
			else
			{
				// Get address of translated code to jump to it
				translation *target_translation = &translation_table[RAM_FLAGS(ptr) >> RFS_TRANSLATION_INDEX];
				uintptr_t jmp_target = reinterpret_cast<uintptr_t>(target_translation->jump_table[ptr - target_translation->start_ptr]);

				// Update PC manually
				emit_mov_imm(W0, addr);
				emit(0xb9003e60); // str w0, [x19, #15*4]
				emit_mov_imm64(X0, jmp_target);
				emit_jmp(reinterpret_cast<void*>(translation_jmp));
			}
		}
		else
			goto unimpl;

		instruction_translated:

		if(cond_branch)
		{
			// Fixup the branch above
			*cond_branch |= ((translate_current - cond_branch) & 0x7FFFF) << 5;
			cond_branch = nullptr;
		}

		RAM_FLAGS(insn_ptr) |= (RF_CODE_TRANSLATED | next_translation_index << RFS_TRANSLATION_INDEX);

		++jump_table_current;
		++insn_ptr;
		pc += 4;
	}

	unimpl:
	// Throw away partial translation
	translate_current = *jump_table_current;
	RAM_FLAGS(insn_ptr) |= RF_CODE_NO_TRANSLATE;

	exit_translation:

	if(insn_ptr == insn_ptr_start)
	{
		// No virtual instruction got translated, just drop everything
		translate_current = translate_buffer_inst_start;
		return;
	}

	if(!jumps_away)
	{
		emit_mov_imm(W0, pc);
		emit_jmp(reinterpret_cast<void*>(translation_next));
	}

	this_translation->end_ptr = insn_ptr;
	this_translation->unused = reinterpret_cast<uintptr_t>(translate_current);

	next_translation_index += 1;

	// Flush the instruction cache
	#ifdef IS_IOS_BUILD
		sys_cache_control(1 /* kCacheFunctionPrepareForExecution */, jump_table_start[0], (translate_current-jump_table_start[0])*4);
	#else
		__builtin___clear_cache(jump_table_start[0], translate_current);
	#endif
}

static void _invalidate_translation(int index)
{
	/* Disabled for now. write_action not called in asmcode_aarch64.S so this can't happen
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
		(void) index;
		flush_translations();
	#else
		_invalidate_translation(index);
	#endif
}

void translate_fix_pc()
{
	if (!translation_sp)
		return;

	uint32_t *insnp = reinterpret_cast<uint32_t*>(try_ptr(arm.reg[15]));
	uint32_t flags = 0;
	if(!insnp || !((flags = RAM_FLAGS(insnp)) & RF_CODE_TRANSLATED))
		return error("Couldn't get PC for fault");

	int index = flags >> RFS_TRANSLATION_INDEX;
	assert(insnp >= translation_table[index].start_ptr);
	assert(insnp < translation_table[index].end_ptr);

	void *ret_pc = translation_sp[-2];

	unsigned int jump_index = insnp - translation_table[index].start_ptr;
	unsigned int translation_insts = translation_table[index].end_ptr - translation_table[index].start_ptr;

	for(unsigned int i = jump_index; ret_pc > translation_table[index].jump_table[i] && i < translation_insts; ++i)
		arm.reg[15] += 4;

	cycle_count_delta -= translation_table[index].end_ptr - insnp;
	translation_sp = nullptr;

	assert(!(arm.cpsr_low28 & 0x20));
}
