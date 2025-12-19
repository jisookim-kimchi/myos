[BITS 32]

global _start
;global problem
global kernel_registers
extern kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ;enable A20 line what is A20 ?: required for accessing memory above 1MB
    in al, 0x92
    or al, 2
    out 0x92, al

    ;enable A20 line what is A20 ?: required for accessing memory above 1MB
    in al, 0x92
    or al, 2
    out 0x92, al

    ;Remap the Master PIC
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

;in kernel there are just 2 selectors, kernel code and kernel data.
;so we can use this selectors to switch to kernel mode.

kernel_registers:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret  

; IDT 테스트를 위한 Division by Zero 인터럽트 발생 함수
;problem:
    ;mov eax, 0    ; EAX 레지스터에 0 저장
    ;div eax       ; 0 ÷ 0 연산 시도 → Division by Zero Exception (인터럽트 0번) 발생!

times 512-($ -$$) db 0 