[BITS 32]

global _start
;global problem
global kernel_registers
extern kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

;to run kernel_main
;ebp : base pointer register, points to base of stack frame
;esp : stack pointer register, points to last stack address
_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ;enable A20 line: required for accessing memory above 1MB
    in al, 0x92
    or al, 2
    out 0x92, al

    ;Remap the Master PIC(PIC : Programmed Interrupt Controller ex keyboard, timer, mouse...)
    mov al, 00010001b   ; ICW1: Initialize PIC, expect ICW4 
    out 0x20, al        ; Send to Master PIC command port
    out 0xA0, al        ; Send to Slave PIC command port
    
    mov al, 0x20        ; ICW2: Master PIC offset (32-39)
    out 0x21, al        ; Send to Master PIC data port
    mov al, 0x28        ; ICW2: Slave PIC offset (40-47)  
    out 0xA1, al        ; Send to Slave PIC data port
    
    mov al, 00000100b   ; ICW3: Master - IRQ2 connects to Slave
    out 0x21, al        ; Send to Master PIC data port
    mov al, 00000010b   ; ICW3: Slave - cascade identity = 2
    out 0xA1, al        ; Send to Slave PIC data port
    
    mov al, 00000001b   ; ICW4: 8086 mode, normal EOI
    out 0x21, al        ; Send to Master PIC data port
    out 0xA1, al        ; Send to Slave PIC data port
    
    mov al, 11111100b   ; Unmask IRQ0(Timer) and IRQ1(Keyboard)
    out 0x21, al        ; Master PIC mask
    mov al, 11111111b   
    out 0xA1, al        ; Slave PIC mask

    call kernel_main
    jmp $

;restore registers to kernel mode
kernel_registers:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret

;fill the rest of the sector with 0
times 512-($ -$$) db 0 