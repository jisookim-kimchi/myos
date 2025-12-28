section .asm

extern interrupt_handler

global get_faulting_address
global interrupt_table
global enable_interrupts
global disable_interrupts
global halt
global idt_load

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

halt:
    hlt
    ret

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]
    lidt [ebx]
    pop ebp    
    ret

; 1. Macro Definitions
%macro INTERRUPT_NO_ERROR_CODE 1
    global interrupt_%1
    interrupt_%1:
        push dword 0      ; Dummy error code
        push dword %1     ; Interrupt Number
        jmp interrupt_common_stub
%endmacro

%macro INTERRUPT_ERROR_CODE 1
    global interrupt_%1
    interrupt_%1:
        push dword %1     ; Interrupt Number (Error code already pushed)
        jmp interrupt_common_stub
%endmacro

; 2. Common Interrupt Stub
interrupt_common_stub:
    ; Create standard stack frame
    push ds
    push es
    push fs
    push gs
    pushad              ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    ; Load Kernel Data Segment
    mov ax, 0x10   
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C handler
    push esp            ; Pass struct interrupt_frame* (points to stack top)
    call interrupt_handler
    add esp, 4          ; Pop argument
    
    ; Restore state
    popad
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Cleanup error code and interrupt number
    add esp, 8
    iret


; exceptions 0~31
INTERRUPT_NO_ERROR_CODE 0
INTERRUPT_NO_ERROR_CODE 1
INTERRUPT_NO_ERROR_CODE 2
INTERRUPT_NO_ERROR_CODE 3
INTERRUPT_NO_ERROR_CODE 4
INTERRUPT_NO_ERROR_CODE 5
INTERRUPT_NO_ERROR_CODE 6
INTERRUPT_NO_ERROR_CODE 7
INTERRUPT_ERROR_CODE    8   ; double fault
INTERRUPT_NO_ERROR_CODE 9
INTERRUPT_ERROR_CODE    10  ; invalid TSS
INTERRUPT_ERROR_CODE    11  ; segment not present
INTERRUPT_ERROR_CODE    12  ; stack-segment fault
INTERRUPT_ERROR_CODE    13  ; general protection fault
INTERRUPT_ERROR_CODE    14  ; page fault
INTERRUPT_NO_ERROR_CODE 15
INTERRUPT_NO_ERROR_CODE 16
INTERRUPT_ERROR_CODE    17  ; alignment check
INTERRUPT_NO_ERROR_CODE 18
INTERRUPT_NO_ERROR_CODE 19
INTERRUPT_NO_ERROR_CODE 20
INTERRUPT_NO_ERROR_CODE 21
INTERRUPT_NO_ERROR_CODE 22
INTERRUPT_NO_ERROR_CODE 23
INTERRUPT_NO_ERROR_CODE 24
INTERRUPT_NO_ERROR_CODE 25
INTERRUPT_NO_ERROR_CODE 26
INTERRUPT_NO_ERROR_CODE 27
INTERRUPT_NO_ERROR_CODE 28
INTERRUPT_NO_ERROR_CODE 29
INTERRUPT_NO_ERROR_CODE 30
INTERRUPT_NO_ERROR_CODE 31

; user interrupts 32~511
%assign i 32
%rep 480
    INTERRUPT_NO_ERROR_CODE i
    %assign i i+1
%endrep

section .data
; pointer table matches C declaration of "interrupt_table"
interrupt_table:
    %assign i 0
    %rep 512
        dd interrupt_%+i
        %assign i i+1
    %endrep


get_faulting_address:
    mov eax, cr2
    ret