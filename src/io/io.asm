section .asm

global insb
global insw
global outsb
global outsw
global inl
global outl

insb:
    push ebp            ;기준점 박아놓기! esp는 움직이니까!
    mov ebp, esp        ;esp는 변경 하면 안됨 항상 최상위 스택을 가리키게해야함!

    xor eax, eax        ; EAX 레지스터 초기화 
    mov edx, [ebp + 8]  ; 포트 번호를 EDX 레지스터에 로드 첫번째 매개변수! 
    in al, dx           ; 포트(dx)에서 바이트 읽기 al: EAX의 하위 8비트 ah: EAX의 상위 8비트
    pop ebp
    ret

insw:
    push ebp
    mov ebp, esp

    xor eax, eax          ; EAX 레지스터 초기화 
    mov edx, [ebp + 8]    ; 포트 번호를 EDX 레지스터에 로드
    in ax, dx             ; 포트에서 워드(2바이트) 읽기 ax: EAX의 하위 16비트
    pop ebp
    ret

outsb:
    push ebp
    mov ebp, esp

    mov eax, [ebp+12]
    mov edx, [ebp+8]
    out dx, al

    pop ebp
    ret

outsw:
    push ebp
    mov ebp, esp

    mov eax, [ebp+12]
    mov edx, [ebp+8]
    out dx, ax

    pop ebp
    ret

inl:
    push ebp
    mov ebp, esp

    xor eax, eax
    mov edx, [ebp + 8]
    in eax, dx
    pop ebp
    ret

outl:
    push ebp
    mov ebp, esp

    mov eax, [ebp+12]
    mov edx, [ebp+8]
    out dx, eax

    pop ebp
    ret

