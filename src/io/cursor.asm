section .asm

global cursor_update

cursor_update:
push ebp
mov ebp, esp
mov eax, [ebp + 8]
mov ecx, eax

mov dx, 0x3d4
mov al, 0x0f
out dx, al

mov dx, 0x3d5
mov al, cl
out dx, al

mov dx, 0x3d4
mov al, 0x0e
out dx, al

mov dx, 0x3d5
mov al, ch
out dx, al

pop ebp
ret