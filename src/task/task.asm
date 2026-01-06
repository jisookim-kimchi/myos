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
    mov ebp, esp
    ; Get the registers pointer from the first argument [ebp+4]
    mov ebx, [ebp+4]
    
    ; Check RPL (low 2 bits) of task's CS to decide on IRET frame size
    mov ax, [ebx+32] ; task->regs.cs
    and ax, 3 ; & 0000 0000 0000 0011 이렇게되네,,,
    cmp ax, 3
    jne .return_to_kernel ; jne : jump if not equal

.return_to_user:
    ; Setup the full IRET frame (SS, ESP, FLAGS, CS, IP) for Ring 3
    push dword [ebx+44] ; ss
    push dword [ebx+40] ; esp
    push dword [ebx+36] ; flags
    push dword [ebx+32] ; cs
    push dword [ebx+28] ; ip
    jmp .restore_common

.return_to_kernel:
    ; Ring 0: iretd pops ONLY FLAGS, CS, EIP.
    ; CRITICAL: Switch to the target task's ESP before pushing the frame!
    mov esp, [ebx+40]
    push dword [ebx+36] ; flags
    push dword [ebx+32] ; cs
    push dword [ebx+28] ; ip

.restore_common:
    ; Restore segment registers (using data selector)
    mov ax, [ebx+44] ; if user, it's 0x23. if kernel, it's 0x10.
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Restore general purpose registers EXCEPT EBX and EBP
    mov edi, [ebx]
    mov esi, [ebx+4]
    mov edx, [ebx+16]
    mov ecx, [ebx+20]
    mov eax, [ebx+24]

    ; Restore EBP then EBX last
    mov ebp, [ebx+8]
    mov ebx, [ebx+12]

    iretd

restore_registers:
    mov ebx, [esp+4] 
    mov edi, [ebx]
    mov esi, [ebx+4]
    mov ebp, [ebx+8]
    mov edx, [ebx+16]
    mov ecx, [ebx+20]
    mov eax, [ebx+24]
    mov ebx, [ebx+12] 
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