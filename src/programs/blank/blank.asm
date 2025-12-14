[BITS 32]

section .asm

%define SYS_PRINT 1
%define SYS_GETKEY 2

global _start

_start:
loop:
    ; 1. Call SYS_GETKEY (Command 2)
    mov eax, SYS_GETKEY
    int 0x80 ;user asking for system call to kernel

    ; 2. Check if key received (EAX != 0)
    cmp eax, 0 ; check if return value is zero
    je loop ; if return value is zero, go back to loop

    ; 3. Store char to buffer
    mov [buffer], al      ; Save the character
    mov byte [buffer+1], 0 ; Null-terminate

    ; 4. Call SYS_PRINT (Command 1)
    push buffer
    mov eax, SYS_PRINT
    int 0x80
    add esp, 4 ; Stack cleanup

    jmp loop

section .data
buffer times 10 db 0
