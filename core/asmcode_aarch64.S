.macro loadsym reg, name
	#ifdef __clang__
		adrp \reg, \name\()@PAGE
		add \reg, \reg, \name\()@PAGEOFF
	#else
		adrp \reg, \name
		add \reg, \reg, #:lo12:\name
	#endif
.endm

.data

.global arm
.global cycle_count_delta
.global cpu_events
.global addr_cache

translation_sp: .global translation_sp
	.xword 0

.text

.global data_abort

translation_enter: .global translation_enter
	stp x19, x30, [sp, #-16]!
	stp x21, x22, [sp, #-16]!
	stp x23, x24, [sp, #-16]!
	stp x25, x26, [sp, #-16]!

	// Store s into translation_sp
	mov x0, sp
	loadsym x1, translation_sp
	str x0, [x1]

	ldr x19, =arm
	loadsym x26, addr_cache
	ldr x26, [x26]

	// Load arm.reg[15] into w0
	ldr w0, [x19, #15*4]

	bl load_virt
	b translation_next_enter

// Enter translation, check for thumb
translation_next_bx: .global translation_next_bx
	tbnz w0, #0, to_thumb // if(pc & 1) goto to_thumb;

// Enter translation; sets arm.reg[15] = w0
translation_next: .global translation_next
	mrs x17, nzcv
	str w0, [x19, #15*4]

// Enter translation at w0 (has to equal arm.reg[15])
translation_next_enter:
	lsr x22, x0, #10
	lsl x22, x22, #4
	ldr x22, [x26, x22] // x22 = addr_cache[(x0 >> 10) << 1]
	tbnz x22, #0, save_return // if(x22 & 1) goto save_return;

	// Load RAM_FLAGS
	add x22, x22, x0 // x22 = pointer to memory at x0
	mov x21, #65*1024*1024
	ldr w21, [x22, x21] // w21 = RAM_FLAGS(x0)
	tbz w21, #5, save_return // if((RAM_FLAGS(x0) & RF_CODE_TRANSLATED) == 0) goto save_return;
	lsr w21, w21, #9 // w21 = w21 >> RFS_TRANSLATION_INDEX

	loadsym x23, cycle_count_delta
	ldr w23, [x23]
	cmp w23, #0
	bpl save_return // if(cycle_count_delta > 0) goto save_return;

	loadsym x23, cpu_events
	ldr w23, [x23]
	cbnz w23, save_return // if(cpu_events) goto save_return;

	loadsym x23, translation_table
	add x23, x23, x21, lsl #5 // x23 = &translation_table[RAM_FLAGS(x0) >> RFS_TRANSLATION_INDEX]

	ldr x24, [x23, #1*8] // x24 = x3->jump_table
	ldp x25, x21, [x23, #2*8] // x25 = x23->start_ptr; x21 = x23->end_ptr

	sub x21, x21, x22 // x21 = end_ptr - insn_ptr
	lsr x21, x21, #2 // x21: number of instructions until the end

	sub x25, x22, x25 // x25 = insn_ptr - start_ptr
	//lsr x25, x25, #2 // x25 = count of instructions
	//lsl x25, x25, #3
	lsl x25, x25, #1
	ldr x0, [x24, x25] // x0 = jump_table[(insn_ptr - start_ptr) / 4]

	// add number of instructions to cycle_count_delta
	loadsym x24, cycle_count_delta
	ldr w25, [x24]
	add w25, w25, w21
	str w25, [x24]

	msr nzcv, x17	
	br x0

translation_jmp: .global translation_jmp
	mrs x17, nzcv

	// add number of instructions to cycle_count_delta
	loadsym x24, cycle_count_delta
	ldr w25, [x24]
	add w25, w25, #4 // We don't know how much will be executed, so use a possible number
	str w25, [x24]
	cmp w25, #0
	bpl save_return

	msr nzcv, x17

	br x0

to_thumb:
	mrs x17, nzcv
	sub w0, w0, #1
	str w0, [x19, #15*4] // arm.reg[15] = w0 - 1
	ldr w1, [x19, #16*4]
	orr w1, w1, #0x20
	str w1, [x19, #16*4] // arm.cpsr_low28 |= 0x20

save_return:
	loadsym x0, translation_sp
	mov x1, #0
	str x1, [x0]

	bl save_virt

	ldp x25, x26, [sp], #16
	ldp x23, x24, [sp], #16
	ldp x21, x22, [sp], #16
	ldp x19, x30, [sp], #16
	ret

// Saves the virtual CPU state
save_virt:
	stp w2, w3, [x19, #0*4]
	stp w4, w5, [x19, #2*4]
	stp w6, w7, [x19, #4*4]
	stp w8, w9, [x19, #6*4]
	stp w10, w11, [x19, #8*4]
	stp w12, w13, [x19, #10*4]
	stp w14, w15, [x19, #12*4]
	str w16, [x19, #14*4]
	// Save nzcv (in x17) to struct arm_state again
	ubfx x2, x17, #31, #1
	ubfx x3, x17, #30, #1
	ubfx x4, x17, #29, #1
	ubfx x5, x17, #28, #1
	strb w2, [x19, #17*4+0]
	strb w3, [x19, #17*4+1]
	strb w4, [x19, #17*4+2]
	strb w5, [x19, #17*4+3]
	ret
	
// Loads the virtual CPU state
// x19 has to be a pointer to the arm_state
load_virt:
	// Assemble virtual cpsr_nzcv in x17
	ldrb w17, [x19, #17*4+0] // x17 = bit 31
	ldrb w3, [x19, #17*4+1] // w3 = bit 30
	ldrb w4, [x19, #17*4+2] // w4 = bit 29
	ldrb w5, [x19, #17*4+3] // w5 = bit 28
	lsl w17, w17, #31
	bfi w17, w3, #30, #1
	bfi w17, w4, #29, #1
	bfi w17, w5, #28, #1
	ldp w2, w3, [x19, #0*4]
	ldp w4, w5, [x19, #2*4]
	ldp w6, w7, [x19, #4*4]
	ldp w8, w9, [x19, #6*4]
	ldp w10, w11, [x19, #8*4]
	ldp w12, w13, [x19, #10*4]
	ldp w14, w15, [x19, #12*4]
	ldr w16, [x19, #14*4]
	ret

read_word_asm: .global read_word_asm
	lsr w22, w0, #10
	add x21, x26, x22, lsl #4 // x21 = &addr_cache[(x0 >> 10) << 1]
0:	ldr x22, [x21] // x22 = *x21
	tbnz x22, #0, 1f
	ldr w0, [x22, x0]
	ret

// Not cached
1:	tbnz x22, #1, 2f
// MMIO
	stp x30, x22, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	bl mmio_read_word
	bl load_virt
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	ret

// Invalid
2:	stp x30, x22, [sp, #-16]!
	stp x0, x1, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	mov x1, #0
	loadsym x2, data_abort
	bl addr_cache_miss
	bl load_virt
	ldp x0, x1, [sp], #16
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	b 0b // Try again

read_half_asm: .global read_half_asm
	bic w22, w22, #1
	lsr w22, w0, #10
	add x21, x26, x22, lsl #4 // x21 = &addr_cache[(x0 >> 10) << 1]
0:	ldr x22, [x21] // x22 = *x21
	tbnz x22, #0, 1f
	ldrh w0, [x22, x0]
	ret

// Not cached
1:	tbnz x22, #1, 2f
// MMIO
	stp x30, x22, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	bl mmio_read_half
	bl load_virt
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	ret

// Invalid
2:	stp x30, x22, [sp, #-16]!
	stp x0, x1, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	mov x1, #0
	loadsym x2, data_abort
	bl addr_cache_miss
	bl load_virt
	ldp x0, x1, [sp], #16
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	b 0b // Try again

read_byte_asm: .global read_byte_asm
	lsr w22, w0, #10
	add x21, x26, x22, lsl #4 // x21 = &addr_cache[(x0 >> 10) << 1]
0:	ldr x22, [x21] // x22 = *x21
	tbnz x22, #0, 1f
	ldrb w0, [x22, x0]
	ret

// Not cached
1:	tbnz x22, #1, 2f
// MMIO
	stp x30, x22, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	bl mmio_read_byte
	bl load_virt
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	ret	

// Invalid
2:	stp x30, x22, [sp, #-16]!
	stp x0, x1, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	mov x1, #0
	loadsym x2, data_abort
	bl addr_cache_miss
	bl load_virt
	ldp x0, x1, [sp], #16
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	b 0b // Try again

write_word_asm: .global write_word_asm
	lsr w22, w0, #10
	add x21, x26, x22, lsl #4 // x21 = &addr_cache[(x0 >> 10) << 1]
0:	ldr x22, [x21, #8] // x22 = *(x21+1)
	tbnz x22, #0, 1f
	str w1, [x22, x0]
	ret

// Not cached
1:	tbnz x22, #1, 2f
// MMIO
	stp x30, x22, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	bl mmio_write_word
	bl load_virt
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	ret

// Invalid
2:	stp x30, x22, [sp, #-16]!
	stp x0, x1, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	mov x1, #1
	loadsym x2, data_abort
	bl addr_cache_miss
	bl load_virt
	ldp x0, x1, [sp], #16
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	b 0b // Try again

write_half_asm: .global write_half_asm
	bic w22, w22, #1
	lsr w22, w0, #10
	add x21, x26, x22, lsl #4 // x21 = &addr_cache[(x0 >> 10) << 1]
0:	ldr x22, [x21, #8] // x22 = *(x21+1)
	tbnz x22, #0, 1f
	strh w1, [x22, x0]
	ret

// Not cached
1:	tbnz x22, #1, 2f
// MMIO
	stp x30, x22, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	bl mmio_write_half
	bl load_virt
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	ret

// Invalid
2:	stp x30, x22, [sp, #-16]!
	stp x0, x1, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	mov x1, #1
	loadsym x2, data_abort
	bl addr_cache_miss
	bl load_virt
	ldp x0, x1, [sp], #16
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	b 0b // Try again

write_byte_asm: .global write_byte_asm
	lsr w22, w0, #10
	add x21, x26, x22, lsl #4 // x21 = &addr_cache[(x0 >> 10) << 1]
0:	ldr x22, [x21, #8] // x22 = *(x21+1)
	tbnz x22, #0, 1f
	strb w1, [x22, x0]
	ret

// Not cached
1:	tbnz x22, #1, 2f
// MMIO
	stp x30, x22, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	bl mmio_write_byte
	bl load_virt
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	ret	

// Invalid
2:	stp x30, x22, [sp, #-16]!
	stp x0, x1, [sp, #-16]!
	mrs x17, nzcv
	bl save_virt
	mov x1, #1
	loadsym x2, data_abort
	bl addr_cache_miss
	bl load_virt
	ldp x0, x1, [sp], #16
	ldp x30, x23, [sp], #16
	msr nzcv, x17	
	b 0b // Try again
