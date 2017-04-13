.data
translation_sp: .global translation_sp
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

/* For now enable .text relocations everywhere. */
#if 1 || defined(__clang__)
/* On iOS, .text relocations are permitted and due to
   a bug it's unable to handle to handle our way of
   avoiding them anyway... */

.macro sym name, no=1
.endm

.macro loadsym reg, name, no=1
    ldr \reg, =\name
.endm
#else

/* What you can see here is a really bad way of avoiding
   relocations in the .text section.
   For each use of an external symbol, you need to insert
   a sym (name), (number) here, which generates a word containing the
   distance to the symbol relative to pc.
   To use it, use loadsym (reg), (name), (number).
   The whole process is a way of working around
   https://sourceware.org/bugzilla/show_bug.cgi?id=18009
*/

.macro sym name, no=1
addr_\name\()_\no\(): .word \name - _tmp_\name\()_\no - 8
.endm

.macro loadsym reg, name, no=1
                   ldr     \reg, addr_\name\()_\no
_tmp_\name\()_\no: add     \reg, \reg, pc
.endm

sym  translation_sp
sym  arm
sym  cycle_count_delta
sym  cpu_events
sym  translation_table
sym  cycle_count_delta, 2
sym  translation_sp, 2
#sym  cpu_events, 2
sym  cycle_count_delta, 3
sym  translation_table, 2
sym  cycle_count_delta, 4
sym  addr_cache
sym  data_abort
sym  addr_cache, 2
sym  data_abort, 2
sym  addr_cache, 3
sym  data_abort, 3
sym  addr_cache, 4
sym  data_abort, 4
sym  addr_cache, 5
sym  data_abort, 5
sym  addr_cache, 6
sym  data_abort, 6

#endif

translation_enter: .global translation_enter
    push    {r4-r11, lr}
    loadsym r0, translation_sp
    str     sp, [r0]
    bl      get_cpsr
    mov     r11, r0 // r11 = cpsr
    loadsym r10, arm // r10 is a pointer to the global arm_state
    ldr     r0, [r10, #15*4] // load arm.reg[15]
    b       translation_next_enter

translation_next_bx: .global translation_next_bx
    tst     r0, #1
    bne     to_thumb

translation_next: .global translation_next
    str     r0, [r10, #15*4] // save to arm.reg[15]

translation_next_enter:
    loadsym r1, cycle_count_delta
    ldr     r1, [r1]
    cmp     r1, #0
    bpl     save_return
    loadsym r1, cpu_events
    ldr     r1, [r1]
    cmp     r1, #0
    bne     save_return

    bl      read_instruction // r0 = pointer to ARM code
    cmp     r0, #0
    beq     save_return

    add     r1, r0, #65*1024*1024 // r1 = &(RAM_FLAGS(r0))
    ldr     r1, [r1]
    tst     r1, #RF_CODE_TRANSLATED
    beq     save_return // not translated

    mov     r1, r1, lsr #RFS_TRANSLATION_INDEX // r1 is translation index
    mov     r1, r1, lsl #4
    loadsym r2, translation_table
    add     r1, r2, r1 // r1 points to struct translation now

    ldr     r2, [r1, #1*4] // load translation.jump_table
    ldr     r3, [r1, #2*4] // load translation.start_ptr
    ldr     r4, [r1, #3*4] // load translation.end_ptr

    sub     r4, r4, r0 // r4 = end_ptr - pc_ptr
    mov     r4, r4, lsr #2 // r4 = number of instructions to the end
    loadsym r6, cycle_count_delta, 2
    ldr     r5, [r6]
    add     r5, r5, r4 // add r4 to cycle_count_delta
    str     r5, [r6]

    sub     r0, r0, r3 // r0 = pc_ptr - start_ptr
    ldr     pc, [r2, r0] // jump to jump_table[r0]

to_thumb:
    sub     r0, r0, #1
    orr     r11, r11, #0x20 // Set thumb bit
    b       save_return

// Invoked from within translated code to jump to other, already translated code
// Target address of translated code is in r0 - only r10 and r11 have to be preserved
// This is used to check for events and leave the translation to process them.
// arm.reg[15] must be set already!
translation_jmp: .global translation_jmp
/*    loadsym r1, cpu_events, 2
    ldr     r2, [r1]
    cmp     r2, #0
    bne     save_return*/

    loadsym r1, cycle_count_delta, 3
    ldr     r2, [r1]
    add     r2, #4 // We don't know how much will be executed, so use a possible number
    str     r2, [r1]
    cmp     r2, #0
    bpl     save_return

    bx      r0 // Jump to the target

translation_jmp_ptr: .global translation_jmp_ptr
    add     r1, r0, #65*1024*1024 // r1 = &(RAM_FLAGS(r0))
    ldr     r1, [r1]
    tst     r1, #RF_CODE_TRANSLATED
    beq     save_return // not translated

    mov     r1, r1, lsr #RFS_TRANSLATION_INDEX // r1 is translation index
    mov     r1, r1, lsl #4
    loadsym r2, translation_table, 2
    add     r1, r2, r1 // r1 points to struct translation now

    ldr     r2, [r1, #1*4] // load translation.jump_table
    ldr     r3, [r1, #2*4] // load translation.start_ptr
    ldr     r4, [r1, #3*4] // load translation.end_ptr

    sub     r4, r4, r0 // r4 = end_ptr - pc_ptr
    mov     r4, r4, lsr #2 // r4 = number of instructions to the end
    loadsym r6, cycle_count_delta, 4
    ldr     r5, [r6]
    add     r5, r5, r4 // add r4 to cycle_count_delta
    str     r5, [r6]

    sub     r0, r0, r3 // r0 = pc_ptr - start_ptr
    ldr     pc, [r2, r0] // jump to jump_table[r0]

// Save flags and leave, arm.reg[15] must be set already!
save_return:
    loadsym r1, translation_sp, 2
    mov     r0, #0
    str     r0, [r1]

    str     r11, [r10, #18*4] // save to arm.cpsr_flags
    mov     r0, r11 // apply arm.cpsr_flags to arm.cpsr_*

    pop     {r4-r11, lr}

    b       set_cpsr_flags

// Below is basically a handcoded assembly version of asmcode.c
//TODO: Invoke write_action for translation invalidation!

write_word_asm: .global write_word_asm
// r0 is address, r1 is value
    loadsym r2, addr_cache
    ldr     r2, [r2]
    mov     r3, r0, lsr #9
    orr     r3, r3, #1
    ldr     r3, [r2, r3, lsl #2] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    streq   r1, [r3, r0]
    bxeq    lr
    str     r11, [r10, #18*4] // save to arm.cpsr_flags
    tst     r3, #AC_INVALID
    bne     write_word_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_write_word
write_word_invalid:
    push    {r0, r1, r2, lr} //r2 for stack alignment
    mov     r1, #1
    loadsym r2, data_abort
    bl      addr_cache_miss
    pop     {r0, r1, r2, lr}
    b       write_word_asm

write_half_asm: .global write_half_asm
// r0 is address, r1 is value
    loadsym r2, addr_cache, 2
    ldr     r2, [r2]
    mov     r3, r0, lsr #9
    orr     r3, r3, #1
    ldr     r3, [r2, r3, lsl #2] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    streqh  r1, [r3, r0]
    bxeq    lr
    str     r11, [r10, #18*4] // save to arm.cpsr_flags
    tst     r3, #AC_INVALID
    bne     write_half_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_write_half
write_half_invalid:
    push    {r0, r1, r2, lr} //r2 for stack alignment
    mov     r1, #1
    loadsym r2, data_abort, 2
    bl      addr_cache_miss
    pop     {r0, r1, r2, lr}
    b       write_half_asm

write_byte_asm: .global write_byte_asm
// r0 is address, r1 is value
    loadsym r2, addr_cache, 3
    ldr     r2, [r2]
    mov     r3, r0, lsr #9
    orr     r3, r3, #1
    ldr     r3, [r2, r3, lsl #2] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    streqb  r1, [r3, r0]
    bxeq    lr
    str     r11, [r10, #18*4] // save to arm.cpsr_flags
    tst     r3, #AC_INVALID
    bne     write_byte_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_write_byte
write_byte_invalid:
    push    {r0, r1, r2, lr} //r2 for stack alignment
    mov     r1, #1
    loadsym r2, data_abort, 3
    bl      addr_cache_miss
    pop     {r0, r1, r2, lr}
    b       write_byte_asm

read_word_asm: .global read_word_asm
// r0 is address
    loadsym r2, addr_cache, 4
    ldr     r2, [r2]
    mov     r3, r0, lsr #10
    ldr     r3, [r2, r3, lsl #3] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    ldreq   r0, [r3, r0]
    bxeq    lr
    str     r11, [r10, #18*4] // save to arm.cpsr_flags
    tst     r3, #AC_INVALID
    bne     read_word_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_read_word
read_word_invalid:
    push    {r0, lr}
    mov     r1, #0
    loadsym r2, data_abort, 4
    bl      addr_cache_miss
    pop     {r0, lr}
    b       read_word_asm

read_half_asm: .global read_half_asm
// r0 is address
    loadsym r2, addr_cache, 5
    ldr     r2, [r2]
    mov     r3, r0, lsr #10
    ldr     r3, [r2, r3, lsl #3] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    ldreqh  r0, [r3, r0]
    bxeq    lr
    str     r11, [r10, #18*4] // save to arm.cpsr_flags
    tst     r3, #AC_INVALID
    bne     read_half_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_read_half
read_half_invalid:
    push    {r0, lr}
    mov     r1, #0
    loadsym r2, data_abort, 5
    bl      addr_cache_miss
    pop     {r0, lr}
    b       read_half_asm

read_byte_asm: .global read_byte_asm
// r0 is address
    loadsym r2, addr_cache, 6
    ldr     r2, [r2]
    mov     r3, r0, lsr #10
    ldr     r3, [r2, r3, lsl #3] // r3 contains ac_entry
    tst     r3, #AC_FLAGS
    ldreqb  r0, [r3, r0]
    bxeq    lr
    str     r11, [r10, #18*4] // save to arm.cpsr_flags
    tst     r3, #AC_INVALID
    bne     read_byte_invalid
    bic     r3, #AC_FLAGS
    add     r0, r3, r0
    b       mmio_read_byte
read_byte_invalid:
    push    {r0, lr}
    mov     r1, #0
    loadsym r2, data_abort, 6
    bl      addr_cache_miss
    pop     {r0, lr}
    b       read_byte_asm
