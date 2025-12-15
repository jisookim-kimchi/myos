section .asm

extern interrupt_handler

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

; 3. Generate Interrupt Stubs
%assign i 0
%rep 512
    INTERRUPT_NO_ERROR_CODE i
    %assign i i+1
%endrep

section .data
; 4. Pointer Table matches C declaration of "interrupt_table"
interrupt_table:
    %assign i 0
    %rep 512
        dd interrupt_%+i
        %assign i i+1
    %endrep