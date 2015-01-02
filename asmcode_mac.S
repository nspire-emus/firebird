// arm_state structure offsets
#define ARM_PC 60
#define ARM_CPSR 64
#define ARM_FLAG_C 70
#define ARM_CONTROL 72

// translation structure offsets
#define TRANS_JUMP_TABLE 0x08
#define TRANS_START_PTR 0x10
#define TRANS_END_PTR 0x18

#define RAM_FLAGS (65*1024*1024) // = MEM_MAXSIZE
#define RF_CODE_TRANSLATED   32
#define RFS_TRANSLATION_INDEX 9

	.text
.globl	translation_enter
translation_enter:
	push	%rbp
	mov		%rsp, %rbp
	push	%rbx
	push	%rsi
	push	%rdi
	movabs	$in_translation_rsp, %r8
	mov 	%rsp, (%r8)

	movabs	$arm, %rbx
	mov		ARM_PC(%rbx), %eax
	jmp		translation_next

.globl	translation_next_bx
translation_next_bx:
	testb	$1, %al
	jne		switch_to_thumb

.globl	translation_next
translation_next:
	mov		%eax, ARM_PC(%rbx)

	movabs	$cycle_count_delta, %r8
	cmpl	$0, (%r8)
	jns		return

	movabs	$cpu_events, %r8
	cmpl	$0, (%r8)
	jnz		return

	mov		ARM_PC(%rbx), %edi
	call	_ptr
	cmp		$0, %rax
	jz		return
addr_ok:

	movl 	RAM_FLAGS(%rax), %edx
	testb	$RF_CODE_TRANSLATED, %dl
	jz		return         // Not translated

	movabs	$in_translation_pc_ptr, %r8
	mov 	%rax, (%r8)

	shr 	$RFS_TRANSLATION_INDEX, %rdx
	shl		$5, %rdx
	movabs	$translation_table, %r8
	add 	%r8, %rdx

	// Add one cycle for each instruction from this point to the end
	mov 	TRANS_END_PTR(%rdx), %rcx
	sub 	%rax, %rcx
	shr 	$2, %rcx
	movabs	$cycle_count_delta, %r8
	add 	%rcx, (%r8)

	mov		%rax, %rcx
	sub		TRANS_START_PTR(%rdx), %rcx
	mov 	TRANS_JUMP_TABLE(%rdx), %rdx
	jmp		*(%rdx, %rcx, 2)
	//That is the same as
	//shr	$2, %rcx
	//jmp	*(%rdx, %rcx, 8)

return:
	movabs	$in_translation_rsp, %r8
	movq 	$0, (%r8)
	pop 	%rdi
	pop 	%rsi
	pop 	%rbx
	pop 	%rbp
	ret

switch_to_thumb:
	dec 	%eax
	mov 	%eax, ARM_PC(%rbx)
	orb		$0x20, ARM_CPSR(%rbx)
	jmp		return

	// These shift procedures are called only from translated code,
	// so they may assume that %rbx == _arm
	.align	4
.globl arm_shift_proc
arm_shift_proc:
	.quad	lsl
	.quad	lsr
	.quad	asr
	.quad	0
	.quad	lsl_carry
	.quad	lsr_carry
	.quad	asr_carry
	.quad	ror_carry

	.text
lsl:
	cmpb	$32, %cl
	jae	ls_32
	shl 	%cl, %eax
	ret

lsr:
	cmpb	$32, %cl
	jae	ls_32
	shr 	%cl, %eax
	ret
ls_32:
	xor 	%eax, %eax
	ret

asr:
	cmpb	$32, %cl
	jae	asr_32
	sar 	%cl, %eax
	ret
asr_32:
	sar 	$31, %eax
	ret

lsl_carry:
	cmpb	$32, %cl
	jae	lsl_carry_32
	testb	%cl, %cl
	je	lsl_carry_zero
	shl 	%cl, %eax
	setc	ARM_FLAG_C(%rbx)
lsl_carry_zero:
	ret
lsl_carry_32:
	jne	ls_carry_33
	shr 	$1, %eax
	setc	ARM_FLAG_C(%rbx)
	xor 	%eax, %eax
	ret

lsr_carry:
	cmpb	$32, %cl
	jae	lsr_carry_32
	testb	%cl, %cl
	je	lsr_carry_zero
	shr 	%cl, %eax
	setc	ARM_FLAG_C(%rbx)
lsr_carry_zero:
	ret
lsr_carry_32:
	jne	ls_carry_33
	shl 	$1, %eax
	setc	ARM_FLAG_C(%rbx)
	xor 	%eax, %eax
	ret
ls_carry_33:
	xor 	%eax, %eax
	movb	%al, ARM_FLAG_C(%rbx)
	ret

asr_carry:
	cmpb	$32, %cl
	jae	asr_carry_32
	testb	%cl, %cl
	je		asr_carry_zero
	sar 	%cl, %eax
	setc	ARM_FLAG_C(%rbx)
asr_carry_zero:
	ret
asr_carry_32:
	sar 	$31, %eax
	sets	ARM_FLAG_C(%rbx)
	ret

ror_carry:
	testb	$31, %cl
	jz	ror_carry_mult_32
	ror 	%cl, %eax
	setc	ARM_FLAG_C(%rbx)
ror_carry_zero:
	ret
ror_carry_mult_32:
	testb	%cl, %cl
	je	ror_carry_zero
	test	%eax, %eax
	sets	ARM_FLAG_C(%rbx)
	ret
