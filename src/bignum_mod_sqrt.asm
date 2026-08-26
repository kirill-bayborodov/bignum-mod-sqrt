; bignum_mod_sqrt.asm
; Independent System V AMD64 YASM implementation.
;
; The optimized path handles normalized one-word operands without C calls. It
; uses 128/64 division for exact modular multiplication, a fast exponentiation
; path for p == 3 (mod 4), and scalar Tonelli-Shanks for p == 1 (mod 4).
; Multi-word records are rejected as capacity/implementation overflow rather
; than being partially modified. Every failure returns before root is written.
;
BITS 64
default rel

%define WORDS       0
%define LEN         256
%define CAPACITY    32
%define SUCCESS     0
%define ERROR_NULL  -1
%define ERROR_MOD   -2
%define ERROR_NROOT -3
%define ERROR_OVER  -4

section .text

global bignum_mod_sqrt
global bignum_mod_sqrt_c11

; uint64_t asm_mulmod(uint64_t a=rdi, uint64_t b=rsi, uint64_t m=rdx)
; Preconditions: a < m, b < m and m != 0. Returns (a*b) % m.
asm_mulmod:
    mov     r8, rdx
    mov     rax, rdi
    mul     rsi
    div     r8
    mov     rax, rdx
    ret

; uint64_t asm_powmod(uint64_t a=rdi, uint64_t e=rsi, uint64_t m=rdx)
; Preconditions: m != 0. Uses a 32-byte aligned local frame for helper calls.
asm_powmod:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32
    mov     [rbp-8], rdx
    mov     [rbp-16], rdi
    mov     [rbp-24], rsi
    mov     rax, 1
    xor     edx, edx
    div     qword [rbp-8]
    mov     [rbp-32], rdx
.pow_loop:
    mov     rax, [rbp-24]
    test    rax, rax
    jz      .pow_done
    test    al, 1
    jz      .pow_skip_mul
    mov     rdi, [rbp-32]
    mov     rsi, [rbp-16]
    mov     rdx, [rbp-8]
    call    asm_mulmod
    mov     [rbp-32], rax
.pow_skip_mul:
    mov     rdi, [rbp-16]
    mov     rsi, [rbp-16]
    mov     rdx, [rbp-8]
    call    asm_mulmod
    mov     [rbp-16], rax
    shr     qword [rbp-24], 1
    jmp     .pow_loop
.pow_done:
    mov     rax, [rbp-32]
    leave
    ret

; bignum_mod_sqrt(const bignum_t *a=rdi, const bignum_t *modulus=rsi,
;                 bignum_t *root=rdx)
; Locals: p=-48, a=-56, q=-64, z=-72, x=-80, t=-88,
;         c=-96, b=-104, i=-112, s=-120. 128-byte frame after saved regs.
bignum_mod_sqrt_c11:
bignum_mod_sqrt:
    test    rdi, rdi
    jz      .null
    test    rsi, rsi
    jz      .null
    test    rdx, rdx
    jz      .null
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 136
    mov     r12, rdx

    mov     r15, [rdi + LEN]
    cmp     r15, CAPACITY
    ja      .overflow
    mov     r13, [rsi + LEN]
    test    r13, r13
    jz      .modulus
    cmp     r13, 1
    ja      .multiword_modulus
    test    r13, r13
    jz      .modulus
    mov     r13, [rsi + WORDS]
    test    r13b, 1
    jz      .modulus
    mov     [rbp-48], r13
    test    r15, r15
    jz      .input_zero
    cmp     r15, 1
    jne     .reduce_input
    mov     r14, [rdi + WORDS]
    xor     edx, edx
    mov     rax, r14
    div     r13
    mov     [rbp-56], rdx
    jmp     .input_reduced
.input_zero:
    mov     qword [rbp-56], 0
    jmp     .input_reduced
.reduce_input:
    xor     r14d, r14d
    mov     r8, r15
.reduce_word:
    dec     r8
    mov     rbx, [rdi + WORDS + r8*8]
    mov     r9d, 63
    mov     ecx, 64
.reduce_bit:
    mov     rax, r14
    xor     r10d, r10d
    shl     rax, 1
    adc     r10, 0
    bt      rbx, r9
    adc     rax, 0
    adc     r10, 0
    mov     rdx, r10
    div     r13
    mov     r14, rdx
    dec     r9
    dec     ecx
    jnz     .reduce_bit
    test    r8, r8
    jnz     .reduce_word
    mov     [rbp-56], r14
.input_reduced:
    mov     r14, [rbp-56]
    test    r14, r14
    jz      .commit_zero

    ; Legendre test: a^((p-1)/2) must be 1 for a nonzero residue.
    mov     rax, r13
    dec     rax
    shr     rax, 1
    mov     rdi, r14
    mov     rsi, rax
    mov     rdx, r13
    call    asm_powmod
    cmp     rax, 1
    jne     .not_residue

    mov     rax, r13
    and     eax, 3
    cmp     eax, 3
    jne     .tonelli
    ; p == 3 (mod 4): x = a^((p+1)/4) mod p.
    mov     rax, r13
    inc     rax
    shr     rax, 2
    mov     rdi, r14
    mov     rsi, rax
    mov     rdx, r13
    call    asm_powmod
    mov     [rbp-80], rax
    jmp     .commit_x

.tonelli:
    ; q = p-1 = qodd * 2^s.
    mov     rax, r13
    dec     rax
    mov     [rbp-64], rax
    xor     ecx, ecx
.factor_two:
    test    al, 1
    jnz     .factor_done
    shr     rax, 1
    inc     ecx
    jmp     .factor_two
.factor_done:
    mov     [rbp-64], rax
    mov     [rbp-120], rcx

    ; Find a quadratic non-residue z, starting at 2.
    mov     qword [rbp-72], 2
.find_z:
    mov     rax, r13
    dec     rax
    shr     rax, 1
    mov     rdi, [rbp-72]
    mov     rsi, rax
    mov     rdx, r13
    call    asm_powmod
    mov     r14, r13
    dec     r14
    cmp     rax, r14
    je      .z_found
    inc     qword [rbp-72]
    jmp     .find_z
.z_found:
    ; x = a^((q+1)/2), c = z^q, t = a^q.
    mov     rax, [rbp-64]
    inc     rax
    shr     rax, 1
    mov     rdi, [rbp-56]
    mov     rsi, rax
    mov     rdx, r13
    call    asm_powmod
    mov     [rbp-80], rax

    mov     rdi, [rbp-72]
    mov     rsi, [rbp-64]
    mov     rdx, r13
    call    asm_powmod
    mov     [rbp-96], rax

    mov     rdi, [rbp-56]
    mov     rsi, [rbp-64]
    mov     rdx, r13
    call    asm_powmod
    mov     [rbp-88], rax

.ts_loop:
    cmp     qword [rbp-88], 1
    je      .commit_x
    mov     qword [rbp-112], 1
    mov     rdi, [rbp-88]
    mov     rsi, [rbp-88]
    mov     rdx, r13
    call    asm_mulmod
    mov     [rbp-104], rax
.find_i:
    cmp     qword [rbp-104], 1
    je      .i_found
    mov     rax, [rbp-112]
    cmp     rax, [rbp-120]
    jae     .not_residue
    mov     rdi, [rbp-104]
    mov     rsi, [rbp-104]
    mov     rdx, r13
    call    asm_mulmod
    mov     [rbp-104], rax
    inc     qword [rbp-112]
    jmp     .find_i
.i_found:
    mov     rbx, [rbp-120]
    sub     rbx, [rbp-112]
    dec     rbx
    mov     r14, [rbp-96]
.raise_b:
    test    rbx, rbx
    jz      .raised_b
    mov     rdi, r14
    mov     rsi, r14
    mov     rdx, r13
    call    asm_mulmod
    mov     r14, rax
    dec     rbx
    jmp     .raise_b
.raised_b:
    mov     [rbp-104], r14
    mov     rdi, r14
    mov     rsi, r14
    mov     rdx, r13
    call    asm_mulmod
    mov     r14, rax
    mov     rdi, [rbp-88]
    mov     rsi, r14
    mov     rdx, r13
    call    asm_mulmod
    mov     [rbp-88], rax
    mov     [rbp-96], r14
    mov     rdi, [rbp-80]
    mov     rsi, [rbp-104]
    mov     rdx, r13
    call    asm_mulmod
    mov     [rbp-80], rax
    mov     rax, [rbp-112]
    mov     [rbp-120], rax
    jmp     .ts_loop

.commit_zero:
    xor     eax, eax
    lea     rdi, [r12 + WORDS]
    mov     ecx, CAPACITY
.zero_loop:
    mov     [rdi], rax
    add     rdi, 8
    dec     ecx
    jnz     .zero_loop
    mov     qword [r12 + LEN], 0
    xor     eax, eax
    jmp     .return
.commit_x:
    mov     rax, [rbp-80]
    mov     [rbp-56], rax
.commit_x_value:
    xor     eax, eax
    lea     rdi, [r12 + WORDS]
    mov     ecx, CAPACITY
.clear_output:
    mov     [rdi], rax
    add     rdi, 8
    dec     ecx
    jnz     .clear_output
    mov     rax, [rbp-56]
    mov     [r12 + WORDS], rax
    mov     qword [r12 + LEN], 1
    xor     eax, eax
.return:
    add     rsp, 136
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret
.null:
    mov     eax, ERROR_NULL
    ret
.multiword_modulus:
    test    r15, r15
    jnz     .overflow
    jmp     .commit_zero
.modulus:
    mov     eax, ERROR_MOD
    jmp     .return
.not_residue:
    mov     eax, ERROR_NROOT
    jmp     .return
.overflow:
    mov     eax, ERROR_OVER
    jmp     .return
