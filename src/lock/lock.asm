[BITS 32]

section .text
  global spin_lock
  global spin_unlock

spin_lock:
  mov edx, [esp + 4]
.loop:
  mov eax, 1
  xchg eax, [edx]
  test eax, eax
  jnz .loop
  ret

spin_unlock:
  mov edx, [esp + 4]
  mov dword [edx], 0
  ret
