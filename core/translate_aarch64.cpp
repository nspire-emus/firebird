/* How the AArch64 translation works:
 * AArch64 has enough registers to keep the entire ARM CPU state available.
 * In JIT'd code, these physical registers have a fixed role:
 * x0: First parameter and return value of helper functions in asmcode_aarch64.S
 * x1: Second parameter to helper functions in asmcode_aarch64.S
 * w2-w16: arm.reg[0] - arm.reg[14]
 * x17: Used as a scratch register for keeping a copy of the virtual cpsr_nzcv
 * (x18: Platform specific, can't use this)
 * x19: Pointer to the arm_state struct
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
#include "os/os.h"

#ifndef __aarch64__
#error "I'm sorry Dave, I'm afraid I can't do that."
#endif

/* Helper functions in asmcode_aarch64.S */
extern "C" {
	extern void translation_next() __asm__("translation_next");
	extern void translation_next_bx() __asm__("translation_next_bx");
	extern void **translation_sp __asm__("translation_sp");
};

enum Reg : uint8_t {
	R0 = 0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, SP, LR, PC
};

enum PReg : uint8_t {
	W0 = 0, W1, W2, W3, W4, W5, W6, W7, W8, W9, W10, W11, W12, W13, W14, W15, W16, W17,
	X0 = W0, X1 = W1, X30 = 30, X31 = 31
};

/* This function returns the physical register that contains the virtual register vreg.
   vreg must not be PC. */
static constexpr PReg mapreg(const uint8_t vreg)
{
	// See the first comment on register mapping
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
	emit(0x52800000 | ((imm & 0xFFFF) << 5) | wd); // movz wd, #imm, lsl #0
	imm >>= 16;
	if(imm)
		emit(0x72a00000 | (imm << 5) | wd); // movk wd, #imm, lsl #16
}

static void emit_mov_reg(const PReg wd, const PReg wm)
{
	emit(0x2a0003e0 | (wm << 16) | wd);
}

// Jumps directly to target, destroys x1
static void emit_jmp(const void *target)
{
	const uint64_t addr = reinterpret_cast<const uint64_t>(target);

	emit(0x58000041); // ldr x1, [pc, #8]
	emit(0xd61f0020); // br x1
	emit(addr & 0xFFFFFFFF); // Lower 32bits of target
	emit(addr >> 32); // Higher 32bits of target
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

		*jump_table_current = translate_current;

		Instruction i;
		i.raw = *insn_ptr;

		if(i.cond != CC_AL && i.cond != CC_NV)
		{
			// Conditional instruction -> Generate jump over code with inverse condition
			cond_branch = translate_current;
			emit(0x54000000 | (i.cond ^ 1));
		}

		if((i.raw & 0xE000090) == 0x0000090)
		{
			goto unimpl;
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
			goto unimpl;
		}
		else if((i.raw & 0xC000000) == 0x4000000)
		{
			goto unimpl;
		}
		else if((i.raw & 0xE000000) == 0x8000000)
		{
			goto unimpl;
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

			emit_mov_imm(W0, addr);
			emit_jmp(reinterpret_cast<void*>(translation_next));
		}
		else
			goto unimpl;

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
	if(!jumps_away)
	{
		emit_mov_imm(W0, pc);
		emit_jmp(reinterpret_cast<void*>(translation_next));
	}

	if(insn_ptr == insn_ptr_start)
	{
		// No virtual instruction got translated, just drop everything
		translate_current = translate_buffer_inst_start;
		return;
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
        flush_translations();
    #else
        _invalidate_translation(index);
    #endif
}

void translate_fix_pc()
{
    if (!translation_sp)
        return;

    // Not implemented: Get accurate pc back in arm.reg[15]
    assert(false);
}
