section .asm

extern int21h_handler
extern int20h_handler
extern no_interrupts_handler

global enable_interrupts
global disable_interrupts
global int21h
global int20h
global idt_load
global no_interrupts

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

idt_load:
    push ebp
    mov ebp, esp ; esp : 스택의 맨 위 데이터
    mov ebx, [ebp + 8] ; 첫 번째 인자: IDT 디스크립터 주소 8: 함수 호출 관리비용.
    lidt [ebx]         ; IDT 로드 명령어
    pop ebp
    ret

int21h:
    cli ; disable interrupts
    pushad ;all registers backup (before interrupt handling)
    call int21h_handler
    popad ; all registers restore (return to original state)
    sti ; enable interrupts
    iret ; return from interrupt (restore EFLAGS, CS, EIP)

int20h:
    cli ; disable interrupts
    pushad ;all registers backup (before interrupt handling)
    call int20h_handler
    popad ; all registers restore (return to original state)
    sti ; enable interrupts
    iret ; return from interrupt (restore EFLAGS, CS, EIP)

no_interrupts:
    cli ; disable interrupts
    pushad ;all registers backup (before interrupt handling)
    call no_interrupts_handler
    popad ; all registers restore (return to original state)
    sti ; enable interrupts
    iret ; return from interrupt (restore EFLAGS, CS, EIP)
