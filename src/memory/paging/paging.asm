[BITS 32]

section .asm

global paging_load_dir
global enable_paging

paging_load_dir:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    mov cr3, eax ; Load the page directory base address into CR3
    pop ebp

    ret

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000 ; Set the paging bit (bit 31
    mov cr0, eax
    pop ebp

    ret