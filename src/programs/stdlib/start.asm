section .asm
[BITS 32]

global _start
extern main
extern exit

_start:
    call main
    push eax
    call exit
    ret
