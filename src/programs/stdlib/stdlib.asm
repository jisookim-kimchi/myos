[BITS 32]

section .asm

global print
global getkey
global sleep

%define SYSCALL_PRINT 1
%define SYSCALL_GET_KEY 2
%define SYSCALL_SLEEP 3

print:
    push ebp
    mov ebp, esp
    push dword[ebp+8]
    mov eax, SYSCALL_PRINT
    int 0x80
    add esp, 4
    pop ebp
    ret

getkey:
    push ebp
    mov ebp, esp
    mov eax, SYSCALL_GET_KEY
    int 0x80
    pop ebp
    ret

sleep:
    push ebp
    mov ebp, esp
    push dword[ebp+8]
    mov eax, SYSCALL_SLEEP
    int 0x80
    add esp, 4
    pop ebp
    ret
