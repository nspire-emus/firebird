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

#ifdef __clang__
    #define streqh strheq
    #define streqb strbeq
    #define ldreqh ldrheq
    #define ldreqb ldrbeq
#endif

#define RF_CODE_TRANSLATED   32
#define RFS_TRANSLATION_INDEX 9

#define AC_INVALID 0b10
#define AC_NOT_PTR 0b01
#define AC_FLAGS (AC_INVALID|AC_NOT_PTR)

translation_enter: .global translation_enter
    push    {r4-r11, lr}
    ldr     r0, =translation_sp
    str     sp, [r0]
    bl      get_cpsr
    mov     r11, r0 // r11 = cpsr
    ldr     lr, =arm // lr is a pointer to the global arm_state
    ldr     r0, [lr, #15*4] // load arm.reg[15]
    b       translation_next

translation_next_bx: .global translation_next_bx
    tst     r0, #1
    bne     to_thumb

translation_next: .global translation_next
    mov     r4, r0
    str     r0, [lr, #15*4] // save to arm.reg[15]
    str     r11, [lr, #18*4] // save to arm.cpsr_flags
    mov     r0, r11 // apply arm.cpsr_flags to arm.cpsr_*
    bl      set_cpsr_flags
    mov     r0, r4

    ldr     r1, =cycle_count_delta
    ldr     r1, [r1]
    cmp     r1, #0
    bpl     return
    ldr     r1, =cpu_events
    ldr     r1, [r1]
    cmp     r1, #0
    bne     return

    bl      read_instruction // r0 = pointer to ARM code
    cmp     r0, #0
    beq     return

    add     r1, r0, #65*1024*1024 // r1 = &(RAM_FLAGS(r0))
    ldr     r1, [r1]
    tst     r1, #RF_CODE_TRANSLATED
    beq     return // not translated

    ldr     r2, =translation_pc_ptr
    str     r0, [r2]

    mov     r1, r1, lsr #RFS_TRANSLATION_INDEX // r1 is translation index
    mov     r1, r1, lsl #4
    ldr     r2, =translation_table
    add     r1, r2, r1 // r1 points to struct translation now

    ldr     r2, [r1, #1*4] // load translation.jump_table
    ldr     r3, [r1, #2*4] // load translation.start_ptr
    ldr     r4, [r1, #3*4] // load translation.end_ptr

    sub     r4, r4, r0 // r4 = end_ptr - pc_ptr
    mov     r4, r4, lsr #2 // r4 = number of instructions to the end
    ldr     r6, =cycle_count_delta
    ldr     r5, [r6]
    add     r5, r5, r4 // add r4 to cycle_count_delta
    str     r5, [r6]

    sub     r0, r0, r3 // r0 = pc_ptr - start_ptr
    ldr     lr, =arm // lr is a pointer to the global arm_state
    ldr     pc, [r2, r0] // jump to jump_table[r0]

to_thumb:
    sub     r0, r0, #1
    orr     r11, r11, #0x20 // Set thumb bit
    b       return

return:
    mov     r0, #0
    ldr     r1, =translation_sp
    str     r0, [r1]

    pop     {r4-r11, pc}

// Below is basically a handcoded assembly version of asmcode.c
//TODO: Invoke write_action for translation invalidation!

write_word: .global write_word
// r0 is address, r1 is value
    ldr     r2, =addr_cache
    ldr     r2, [r2]
    mov     r3, r0, lsr #9
    orr     r3, r3, #1
    ldr     r3, [r2, r3, lsl #2] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    streq   r1, [r3, r0]
    bxeq    lr
    tst     r3, #AC_INVALID
    bne     write_word_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_write_word
write_word_invalid:
    push    {r0, r1, r2, lr} //r2 for stack alignment
    mov     r1, #1
    ldr     r2, =data_abort
    bl      addr_cache_miss
    pop     {r0, r1, r2, lr}
    b       write_word

write_half: .global write_half
// r0 is address, r1 is value
    ldr     r2, =addr_cache
    ldr     r2, [r2]
    mov     r3, r0, lsr #9
    orr     r3, r3, #1
    ldr     r3, [r2, r3, lsl #2] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    streqh  r1, [r3, r0]
    bxeq    lr
    tst     r3, #AC_INVALID
    bne     write_half_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_write_half
write_half_invalid:
    push    {r0, r1, r2, lr} //r2 for stack alignment
    mov     r1, #1
    ldr     r2, =data_abort
    bl      addr_cache_miss
    pop     {r0, r1, r2, lr}
    b       write_half


write_byte: .global write_byte
// r0 is address, r1 is value
    ldr     r2, =addr_cache
    ldr     r2, [r2]
    mov     r3, r0, lsr #9
    orr     r3, r3, #1
    ldr     r3, [r2, r3, lsl #2] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    streqb  r1, [r3, r0]
    bxeq    lr
    tst     r3, #AC_INVALID
    bne     write_byte_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_write_byte
write_byte_invalid:
    push    {r0, r1, r2, lr} //r2 for stack alignment
    mov     r1, #1
    ldr     r2, =data_abort
    bl      addr_cache_miss
    pop     {r0, r1, r2, lr}
    b       write_byte

read_word_ldr: .global read_word_ldr
read_word: .global read_word
// r0 is address
    ldr     r2, =addr_cache
    ldr     r2, [r2]
    mov     r3, r0, lsr #9
    bic     r3, r3, #1
    ldr     r3, [r2, r3, lsl #2] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    ldreq   r0, [r3, r0]
    bxeq    lr
    tst     r3, #AC_INVALID
    bne     read_word_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_read_word
read_word_invalid:
    push    {r0, lr}
    mov     r1, #0
    ldr     r2, =data_abort
    bl      addr_cache_miss
    pop     {r0, lr}
    b       read_word

read_half: .global read_half
// r0 is address
    ldr     r2, =addr_cache
    ldr     r2, [r2]
    mov     r3, r0, lsr #9
    bic     r3, r3, #1
    ldr     r3, [r2, r3, lsl #2] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    ldreqh  r0, [r3, r0]
    bxeq    lr
    tst     r3, #AC_INVALID
    bne     read_half_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_read_half
read_half_invalid:
    push    {r0, lr}
    mov     r1, #0
    ldr     r2, =data_abort
    bl      addr_cache_miss
    pop     {r0, lr}
    b       read_half


read_byte: .global read_byte
// r0 is address
    ldr     r2, =addr_cache
    ldr     r2, [r2]
    mov     r3, r0, lsr #9
    bic     r3, r3, #1
    ldr     r3, [r2, r3, lsl #2] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    ldreqb  r0, [r3, r0]
    bxeq    lr
    tst     r3, #AC_INVALID
    bne     read_byte_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_read_byte
read_byte_invalid:
    push    {r0, lr}
    mov     r1, #0
    ldr     r2, =data_abort
    bl      addr_cache_miss
    pop     {r0, lr}
    b       read_byte
