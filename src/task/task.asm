[BITS 32]
section .asm

global restore_registers
global task_return
global user_registers

;테스크 전환, 유저모드 진입.

;iret 프레임이란?
;iret(또는 iretd) 가 실행될 때 CPU는 스택에 미리 준비된 5개의 값을 차례대로 꺼내서 레지스터와 특권 레벨을 복원합니다. 
;이 5개의 값이 “iret 프레임” 입니다.

task_return:
    ; make iret frames
    ; PUSH THE DATA SEGMENT (SS WILL BE FINE)
    ; PUSH THE STACK ADDRESS
    ; PUSH THE FLAGS
    ; PUSH THE CODE SEGMENT
    ; PUSH IP

    ; Let's access the structure passed to us
    mov ebx, [esp+4] ;get the first argument passed to us
    ; push the data/stack selector
    push dword [ebx+44] ;in structure task, we push the ss because it is offset 44
    ; Push the stack pointer
    push dword [ebx+40] ;in structure task, we push the esp because it is offset 40

    ; Push the flags
    pushf
    pop eax
    or eax, 0x200
    push eax

    ; Push the code segment
    push dword [ebx+32]

    ; Push the IP to execute
    push dword [ebx+28]

    ; Setup some segment registers
    mov ax, [ebx+44]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword [esp+24]
    call restore_registers
    add esp, 4

    ; Let's leave kernel land and execute in user land!
    iretd

restore_registers:
    push ebp  ;이전의 함수 베이스프레임 push 하면 esp-4 됨.
    mov ebp, esp ;근데 난 현재 함수의 베이스프레임을 저장할 거야. ebp: 베이스프레임 포인터 esp : 스택프레임에서 최고 상단을 가리키는 포인터.
    mov ebx, [ebp+8] ;  esp +4는 첫번째인잔데 -4감소햇으니 +8
    mov edi, [ebx]
    mov esi, [ebx+4]
    mov ebp, [ebx+8]
    mov edx, [ebx+16]
    mov ecx, [ebx+20]
    mov eax, [ebx+24]
    mov ebx, [ebx+12]
    pop ebp  ;이전 ebp 의 값을 새 스택프레임 esp 로 복구시킨다. pop: 스택 최상단 esp가 가리키는 주소를 읽는다. 그리고 esp 에 넣는다 그리고 esp 를 4바이트증가시킴.
    ret


;하는 일: CPU의 데이터 세그먼트 레지스터들(DS, ES, FS, GS) 을 사용자 모드용 값(0x23) 으로 설정합니다.
;의미: "이제부터 데이터를 읽거나 쓸 때는 사용자 권한의 데이터 영역을 참조해라"라고 CPU에게 알려주는 것입니다. 커널이 사용자 메모리에 접근해야 하거나, 곧 사용자 모드로 넘어갈 준비를 하는 단계입니다.
user_registers:
    mov ax, 0x23 ;user data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret