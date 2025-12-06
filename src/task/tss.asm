section .asm

global tss_load

;for changing to kernel mode
tss_load:
    push ebp
    mov ebp, esp
    mov ax, [ebp+8] ; TSS Segment
    ltr ax
    pop ebp
    ret