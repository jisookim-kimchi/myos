section .asm
global gdt_load

;return 주소는 스택에 저장되어있음 항상 .
;return주소는 esp
;함수에서 첫번째 인자는 esp+4 , 두번째 인자는 esp+8 왜 4의 배수? : 32비트 = 4바이트 라서, 
;eax is 32bit register
gdt_load:
    mov eax, [esp+4]
    mov [gdt_descriptor + 2], eax ;gdt_descriptor 메모리블록안에서 2바이트 뒤를 의미. 즉 아래보면 size가 2바이트니까 거기를 건너뛰어서!
    mov ax, [esp+8]
    mov [gdt_descriptor], ax ;gdt_descriptor 메모리블록안에서 2바이트 앞을 의미.
    lgdt [gdt_descriptor]
    ret


section .data
gdt_descriptor:
    dw 0x00 ; Size ;2bytes  
    dd 0x00 ; GDT Start Address ;4bytes