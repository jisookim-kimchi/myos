[BITS 32]

section .asm

global print
global getkey
global sleep
global fopen
global sum
global fread

%define SYSCALL_SUM 0
%define SYSCALL_PRINT 1
%define SYSCALL_GET_KEY 2
%define SYSCALL_SLEEP 3
%define SYSCALL_OPEN 4
%define SYSCALL_READ 5

sum:
    mov eax, SYSCALL_SUM
    int 0x80
    ret

print:
    mov eax, SYSCALL_PRINT
    int 0x80
    ret

getkey:
    mov eax, SYSCALL_GET_KEY
    int 0x80
    ret

sleep:
    mov eax, SYSCALL_SLEEP
    int 0x80
    ret

fopen:
    mov eax, SYSCALL_OPEN
    int 0x80
    ret

fread:
    mov eax, SYSCALL_READ
    int 0x80
    ret
