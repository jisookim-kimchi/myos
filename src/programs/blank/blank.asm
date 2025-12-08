[BITS 32]

section .asm

%define SYS_SUM 0

global _start

_start:

label:
    mov eax, SYS_SUM
    mov ebx, 10
    mov ecx, 20
    int 0x80

    jmp $