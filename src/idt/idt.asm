section .asm

extern int21h_handler
;extern int20h_handler
extern no_interrupts_handler
extern isr80h_handler

global enable_interrupts
global disable_interrupts
global int21h
global int20h
global idt_load
global no_interrupts
global isr80h_wrapper

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
    ;iret ; return from interrupt (restore EFLAGS, CS, EIP)

;int20h:
    ;cli ; disable interrupts
    ;pushad ;all registers backup (before interrupt handling)
    ;call int20h_handler
    ;popad ; all registers restore (return to original state)
    ;sti ; enable interrupts
    ;iret ; return from interrupt (restore EFLAGS, CS, EIP)

no_interrupts:
    cli ; disable interrupts
    pushad ;all registers backup (before interrupt handling)
    call no_interrupts_handler
    popad ; all registers restore (return to original state)
    sti ; enable interrupts
    iret ; return from interrupt (restore EFLAGS, CS, EIP)

isr80h_wrapper:
    cli                 ; 인터럽트 끄기 (방해 금지)
    
    pushad              ; 모든 레지스터(eax, ebx...)를 스택에 백업
                        ; 왜? 커널이 일하다가 레지스터 값을 바꿀 수 있으니까,
                        ; 나중에 유저한테 돌아갈 때 복구해주려고.
    
    push esp            ; 현재 스택 포인터(esp)를 인자로 넘김
                        ; 이렇게 하면 C 함수에서 스택에 저장된 레지스터 값들을 읽을 수 있음
                        ; (struct registers* 처럼 접근 가능)
    
    call isr80h_handler ; C 함수 호출! (이제 진짜 일을 하러 감)
                        ; 이 함수는 아직 안 만들었습니다. 이제 만들 겁니다.
    
    add esp, 4          ; 인자(push esp) 제거
    
    ; 중요: C 함수가 리턴한 값(eax)을 유저에게 전달해야 함.
    ;C 언어(그리고 대부분의 컴파일러)의 약속(Calling Convention) 에 따르면: 함수의 리턴값(Return Value) 은 항상 EAX 레지스터에 저장됩니다.
    ; 하지만 밑에서 popad를 하면 백업해둔 옛날 eax 값으로 덮어씌워짐.
    ; 그래서 스택에 백업된 eax 자리에, 방금 받은 새 리턴값을 덮어쓰는 것임.

    mov [esp + 28], eax 
    popad               ; 백업해둔 레지스터 복구
    sti                 ; 인터럽트 켜기
    iret                ; 유저 모드로 복귀!