global mutex_lock
global mutex_unlock

mutex_lock:
    mov edx, [esp + 4]
    .retry:
    mov eax, 1
    xchg [edx], eax
    test eax, eax
    jnz .retry
    ret

mutex_unlock:
    mov edx, [esp + 4]
    mov dword [edx], 0
    ret