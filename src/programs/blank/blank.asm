[BITS 32]

section .asm

%define SYS_SUM 0

global _start

_start:

label:
    push 20
    push 10
    mov eax, SYS_SUM
    int 0x80
    add esp,8

    jmp $