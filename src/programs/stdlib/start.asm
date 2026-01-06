section .asm
[BITS 32]

global _start
extern main
extern exit

_start:
    add esp, 4
    call main
    push eax
    call exit
    ret
