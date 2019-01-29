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
#define RF_READ_BREAKPOINT   1
#define RF_WRITE_BREAKPOINT  2
#define RF_EXEC_BREAKPOINT   4
#define RF_EXEC_DEBUG_NEXT   8
#define RF_CODE_TRANSLATED   32
#define RF_CODE_NO_TRANSLATE 64
#define RF_READ_ONLY         128
#define RF_ARMLOADER_CB      256
#define RFS_TRANSLATION_INDEX 9

#define DO_READ_ACTION (RF_READ_BREAKPOINT)
#define DO_WRITE_ACTION (RF_WRITE_BREAKPOINT | RF_CODE_TRANSLATED | RF_CODE_NO_TRANSLATE)

translation_enter: .global translation_enter
    push    %rbp
    mov     %rsp, %rbp
    push    %rbx
    push    %rsi
    push    %rdi
    mov     %rsp, in_translation_rsp(%rip)

    lea     arm(%rip), %rbx
    mov     ARM_PC(%rbx), %eax
    jmp     translation_next

translation_next_bx: .global translation_next_bx
    testb   $1, %al
    jne     switch_to_thumb

translation_next: .global translation_next
    mov     %eax, ARM_PC(%rbx)

    lea     cycle_count_delta(%rip), %r8
    cmpl    $0, (%r8)
    jns     return

    lea     cpu_events(%rip), %r8
    cmpl    $0, (%r8)
    jnz     return

    mov     ARM_PC(%rbx), %edi
    push    %rdi // For 16 byte stack alignment (call pushes 8 itself)
    call    read_instruction
    pop     %rdi
    cmp     $0, %rax
    jz      return

addr_ok:
    movl    RAM_FLAGS(%rax), %edx
    testb   $RF_CODE_TRANSLATED, %dl
    jz      return         // Not translated

    lea     in_translation_pc_ptr(%rip), %r8
    mov     %rax, (%r8)

    shr     $RFS_TRANSLATION_INDEX, %rdx
    shl     $5, %rdx
    lea     translation_table(%rip), %r8
    add     %r8, %rdx

    // Add one cycle for each instruction from this point to the end
    mov     TRANS_END_PTR(%rdx), %rcx
    sub     %rax, %rcx
    shr     $2, %rcx
    lea     cycle_count_delta(%rip), %r8
    add     %ecx, (%r8)

    mov     %rax, %rcx
    sub     TRANS_START_PTR(%rdx), %rcx
    mov     TRANS_JUMP_TABLE(%rdx), %rdx
    jmp     *(%rdx, %rcx, 2)
    //That is the same as
    //shr    $2, %rcx
    //jmp    *(%rdx, %rcx, 8)

return:
    lea     in_translation_rsp(%rip), %r8
    movq    $0, (%r8)
    pop     %rdi
    pop     %rsi
    pop     %rbx
    pop     %rbp
    ret

switch_to_thumb:
    dec     %eax
    mov     %eax, ARM_PC(%rbx)
    orb     $0x20, ARM_CPSR(%rbx)
    jmp     return

    .data
    // These shift procedures are called only from translated code,
    // so they may assume that %rbx == _arm
    .align 4
arm_shift_proc: .global arm_shift_proc
    .quad   lsl
    .quad   lsr
    .quad   asr
    .quad   0
    .quad   lsl_carry
    .quad   lsr_carry
    .quad   asr_carry
    .quad   ror_carry

    .text
lsl:
    cmpb    $32, %cl
    jae     ls_32
    shl     %cl, %eax
    ret

lsr:
    cmpb    $32, %cl
    jae     ls_32
    shr     %cl, %eax
    ret
ls_32:
    xor     %eax, %eax
    ret

asr:
    cmpb    $32, %cl
    jae     asr_32
    sar     %cl, %eax
    ret
asr_32:
    sar     $31, %eax
    ret

lsl_carry:
    cmpb    $32, %cl
    jae     lsl_carry_32
    testb   %cl, %cl
    je      lsl_carry_zero
    shl     %cl, %eax
    setc    ARM_FLAG_C(%rbx)
lsl_carry_zero:
    ret
lsl_carry_32:
    jne     ls_carry_33
    shr     $1, %eax
    setc    ARM_FLAG_C(%rbx)
    xor     %eax, %eax
    ret

lsr_carry:
    cmpb    $32, %cl
    jae     lsr_carry_32
    testb   %cl, %cl
    je      lsr_carry_zero
    shr     %cl, %eax
    setc    ARM_FLAG_C(%rbx)
lsr_carry_zero:
    ret
lsr_carry_32:
    jne     ls_carry_33
    shl     $1, %eax
    setc    ARM_FLAG_C(%rbx)
    xor     %eax, %eax
    ret
ls_carry_33:
    xor     %eax, %eax
    movb    %al, ARM_FLAG_C(%rbx)
    ret

asr_carry:
    cmpb    $32, %cl
    jae     asr_carry_32
    testb   %cl, %cl
    je      asr_carry_zero
    sar     %cl, %eax
    setc    ARM_FLAG_C(%rbx)
asr_carry_zero:
    ret
asr_carry_32:
    sar     $31, %eax
    sets    ARM_FLAG_C(%rbx)
    ret

ror_carry:
    testb   $31, %cl
    jz      ror_carry_mult_32
    ror     %cl, %eax
    setc    ARM_FLAG_C(%rbx)
ror_carry_zero:
    ret
ror_carry_mult_32:
    testb   %cl, %cl
    je      ror_carry_zero
    test    %eax, %eax
    sets    ARM_FLAG_C(%rbx)
    ret

read_word_asm: .global read_word_asm
    mov     %rdi, %rax
    shr     $10, %rax
    shl     $1, %rax
    mov     addr_cache(%rip), %r8
    mov     (%r8, %rax, 8), %rax
    test    $3, %rax
    jnz     rwa_miss
    movl    (%rax, %rdi), %eax
    ret
rwa_miss:
    push    %rdx
    push    %rcx
    call    read_word
    pop     %rcx
    pop     %rdx
    ret

write_word_asm: .global write_word_asm
    mov     %rdi, %rax
    shr     $10, %rax
    shl     $1, %rax
    add     $1, %rax
    mov     addr_cache(%rip), %r8
    mov     (%r8, %rax, 8), %rax
    test    $3, %rax
    jnz     wwa_miss
    movl    %esi, (%rax, %rdi)
    testq   $DO_WRITE_ACTION, RAM_FLAGS(%rax, %rdi)
    jnz     write_action_asm
    ret
wwa_miss:
    push    %rdx
    push    %rcx
    call    write_word
    pop     %rcx
    pop     %rdx
    ret

read_half_asm: .global read_half_asm
    and     $-2, %rdi
    mov     %rdi, %rax
    shr     $10, %rax
    shl     $1, %rax
    mov     addr_cache(%rip), %r8
    mov     (%r8, %rax, 8), %rax
    test    $3, %rax
    jnz     rha_miss
    movzwl  (%rax, %rdi), %eax
    ret
rha_miss:
    push    %rdx
    push    %rcx
    call    read_half
    pop     %rcx
    pop     %rdx
    ret

write_half_asm: .global write_half_asm
    and     $-2, %rdi
    mov     %rdi, %rax
    shr     $10, %rax
    shl     $1, %rax
    add     $1, %rax
    mov     addr_cache(%rip), %r8
    mov     (%r8, %rax, 8), %rax
    test    $3, %rax
    jnz     wha_miss
    movw    %si, (%rax, %rdi)
    testq   $DO_WRITE_ACTION, RAM_FLAGS(%rax, %rdi)
    jnz     write_action_asm
    ret
wha_miss:
    push    %rdx
    push    %rcx
    call    write_half
    pop     %rcx
    pop     %rdx
    ret

read_byte_asm: .global read_byte_asm
    mov     %rdi, %rax
    shr     $10, %rax
    shl     $1, %rax
    mov     addr_cache(%rip), %r8
    mov     (%r8, %rax, 8), %rax
    test    $3, %rax
    jnz     rba_miss
    movzb   (%rax, %rdi), %rax
    ret
rba_miss:
    push    %rdx
    push    %rcx
    call    read_byte
    pop     %rcx
    pop     %rdx
    ret

write_byte_asm: .global write_byte_asm
    mov     %rdi, %rax
    shr     $10, %rax
    shl     $1, %rax
    add     $1, %rax
    mov     addr_cache(%rip), %r8
    mov     (%r8, %rax, 8), %rax
    test    $3, %rax
    jnz     wba_miss
    xchg    %rsi, %rdx // Can't use %rsi directly
    movb    %dl, (%rax, %rdi)
    xchg    %rsi, %rdx
    testq   $DO_WRITE_ACTION, RAM_FLAGS(%rax, %rdi)
    jnz     write_action_asm
    ret
wba_miss:
    push    %rdx
    push    %rcx
    call    write_byte
    pop     %rcx
    pop     %rdx
    ret

write_action_asm:
    add     %rax, %rdi
    push    %rdx
    push    %rcx
    call    write_action
    pop     %rcx
    pop     %rdx
    ret
