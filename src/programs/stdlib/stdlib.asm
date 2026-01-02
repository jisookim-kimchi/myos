[BITS 32]

section .asm

global print
global getkey
global sleep
global fopen
global fread
global sbrk
global fclose
global fwrite
global exit
global wait_pid
global exec
global fstat

%define SYSCALL_PRINT 1
%define SYSCALL_GET_KEY 2
%define SYSCALL_SLEEP 3
%define SYSCALL_OPEN 4
%define SYSCALL_READ 5
%define SYSCALL_SBRK 6
%define SYSCALL_CLOSE 7
%define SYSCALL_WRITE 8
%define SYSCALL_EXIT 9
%define SYSCALL_WAIT 10
%define SYSCALL_EXEC 11
%define SYSCALL_FSTAT 12

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

sbrk:
    mov eax, SYSCALL_SBRK
    int 0x80
    ret

fclose:
    mov eax, SYSCALL_CLOSE
    int 0x80
    ret

fwrite:
    mov eax, SYSCALL_WRITE
    int 0x80
    ret

exit:
    mov eax, SYSCALL_EXIT
    int 0x80
    ret

wait_pid:
    mov eax, SYSCALL_WAIT
    int 0x80
    ret

exec:
    mov eax, SYSCALL_EXEC
    int 0x80
    ret

fstat:
    mov eax, SYSCALL_FSTAT
    int 0x80
    ret
