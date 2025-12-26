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

    push ebx
    call restore_registers
    add esp, 4

    ; Let's leave kernel land and execute in user land!
    iretd

restore_registers:
    mov ebx, [esp+4] ; Load the pointer to the registers struct (1st argument)
    mov edi, [ebx]
    mov esi, [ebx+4]
    mov ebp, [ebx+8]
    mov edx, [ebx+16]
    mov ecx, [ebx+20]
    mov eax, [ebx+24]
    mov ebx, [ebx+12] ; Restore EBX last!
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