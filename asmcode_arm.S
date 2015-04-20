.data
translation_sp: .global translation_sp
	.word 0
translation_pc_ptr: .global translation_pc_ptr
	.word 0
.global arm
.global get_cpsr_flags
.global set_cpsr_flags
.global cycle_count_delta
.global cpu_events
.global read_instruction

.text

#define RF_CODE_TRANSLATED   32
#define RFS_TRANSLATION_INDEX 9

translation_enter: .global translation_enter
        push		{r4-r12, lr}
	ldr		r0, =translation_sp
	str		sp, [r0]
	bl		get_cpsr_flags
	mov		r12, r0 // r12 = cpsr
	ldr		lr, =arm // lr is a pointer to the global arm_state
	ldr		r0, [lr, #15*4] // load arm.reg[15]
	b		translation_next

translation_next_bx: .global translation_next_bx
	tst		r0, #1
	bne		to_thumb

translation_next: .global translation_next
	str		r0, [lr, #15*4] // save to arm.reg[15]

	ldr		r1, =cycle_count_delta
	ldr		r1, [r1]
	cmp		r1, #0
	bpl		return
	ldr		r1, =cpu_events
	ldr		r1, [r1]
	cmp		r1, #0
	bne		return

	bl		read_instruction // r0 = pointer to ARM code
	cmp		r0, #0
	beq		return

	add		r1, r0, #65*1024*1024 // r1 = &(RAM_FLAGS(r0))
	ldr		r1, [r1]
	tst		r1, #RF_CODE_TRANSLATED
	beq		return // not translated

	ldr		r2, =translation_pc_ptr
	str		r0, [r2]

	mov		r1, r1, lsr #RFS_TRANSLATION_INDEX // r1 is translation index
	mov		r1, r1, lsl #4
	ldr		r2, =translation_table
	add		r1, r2, r1 // r1 points to struct translation now

	ldr		r2, [r1, #1*4] // load translation.jump_table
	ldr		r3, [r1, #2*4] // load translation.start_ptr
	ldr		r4, [r1, #3*4] // load translation.end_ptr

	sub		r4, r4, r0 // r4 = end_ptr - pc_ptr
	mov		r4, r4, lsr #2 // r4 = number of instructions to the end
	ldr		r6, =cycle_count_delta
	ldr		r5, [r6]
	add		r5, r5, r4 // add r4 to cycle_count_delta
	str		r5, [r6]

	sub		r0, r0, r3 // r0 = pc_ptr - start_ptr
        ldr		lr, =arm // lr is a pointer to the global arm_state
	ldr		pc, [r2, r0] // jump to jump_table[r0]

to_thumb:
	sub		r0, r0, #1
	orr		r12, r12, #0x20 // Set thumb bit
	b		return

return:
	mov		r0, #0
	ldr		r1, =translation_sp
	str		r0, [r1]

	pop		{r4-r12, pc}

/*read_word: .global read_word
read_word_ldr: .global read_word_ldr
	mov		r1, r0, lsr #10
	adr		r2, addr_cache
	ldr		r2, [r2]
	ldr		r1, [r2, r1, lsl #1]
	tst		r1, #0b11
	ldreq	r0, [r1, r0]
	bx		lr
	tst		r1, #0b10
	beq		phys_word
	mov		r1, #0
	push	{r4, lr}
	mov		r4, r0
	bl		addr_cache_miss
	mov		r0, r4
	pop		{r4, lr}
	b		read_word
phys_word:
	mvn		r2, #0b11
	and		r1, r1, r2
	add		r0, r1, r0
	pop		{r2, r3}
	b		mmio_read_word

read_half:  .global read_half
	mov		r1, r0, lsr #10
	adr		r2, addr_cache
	ldr		r2, [r2]
	ldr		r1, [r2, r1, lsl #1]
	tst		r1, #0b11
	ldreqh	r0, [r1, r0]
	bx		lr
	tst		r1, #0b10
	beq		phys_half
	mov		r1, #0
	push	{r4, lr}
	mov		r4, r0
	bl		addr_cache_miss
	mov		r0, r4
	pop		{r4, lr}
	b		read_half
phys_half:
	mvn		r2, #0b11
	and		r1, r1, r2
	add		r0, r1, r0
	pop		{r2, r3}
	b		mmio_read_half

read_byte:  .global read_byte
	mov		r1, r0, lsr #10
	adr		r2, addr_cache
	ldr		r2, [r2]
	ldr		r1, [r2, r1, lsl #1]
	tst		r1, #0b11
	ldreqb	r0, [r1, r0]
	bx		lr
	tst		r1, #0b10
	beq		phys_half
	mov		r1, #0
	push	{r4, lr}
	mov		r4, r0
	bl		addr_cache_miss
	mov		r0, r4
	pop		{r4, lr}
	b		read_byte
phys_byte:
	mvn		r2, #0b11
	and		r1, r1, r2
	add		r0, r1, r0
	pop		{r2, r3}
	b		mmio_read_byte

write_word: .global write_word
	mov		r2, r0, lsr #10
	adr		r3, addr_cache
	ldr		r3, [r3]
	ldr		r2, [r3, r2, lsl #1]
	tst		r2, #0b11
	streq	r1, [r2, r0]
	bx		lr
	tst		r2, #0b10
	beq		phys_wword
	push	{r0, r1, r4, lr}
	mov		r1, #1
	bl		addr_cache_miss
	pop		{r0, r1, r4, lr}
	b		write_word
phys_wword:
	mvn		r3, #0b11
	and		r2, r2, r2
	add		r0, r2, r0
	pop		{r2, r3}
	b		mmio_write_word

write_half: .global write_half
	mov		r2, r0, lsr #10
	adr		r3, addr_cache
	ldr		r3, [r3]
	ldr		r2, [r3, r2, lsl #1]
	tst		r2, #0b11
	streqh	r1, [r2, r0]
	bx		lr
	tst		r2, #0b10
	beq		phys_whalf
	push	{r0, r1, r4, lr}
	mov		r1, #1
	bl		addr_cache_miss
	pop		{r0, r1, r4, lr}
	b		write_half
phys_whalf:
	mvn		r3, #0b11
	and		r2, r2, r2
	add		r0, r2, r0
	pop		{r2, r3}
	b		mmio_write_half

write_byte: .global write_byte
	mov		r2, r0, lsr #10
	adr		r3, addr_cache
	ldr		r3, [r3]
	ldr		r2, [r3, r2, lsl #1]
	tst		r2, #0b11
	streqb	r1, [r2, r0]
	bx		lr
	tst		r2, #0b10
	beq		phys_wbyte
	push	{r0, r1, r4, lr}
	mov		r1, #1
	bl		addr_cache_miss
	pop		{r0, r1, r4, lr}
	b		write_byte
phys_wbyte:
	mvn		r3, #0b11
	and		r2, r2, r2
	add		r0, r2, r0
	pop		{r2, r3}
	b		mmio_write_byte
*/
