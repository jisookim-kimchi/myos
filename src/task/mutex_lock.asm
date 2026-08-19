section .text

global atomic_exchange
;int a_lock = 0;
;int atomic_exchange(int *ptr, int value)
;ptr

global k_mutex_lock
global k_mutex_unlock

atomic_exchange:
    mov edx, [esp + 4]   
    mov eax, [esp + 8]
    xchg [edx], eax         ;must to use xchg other dont' use cmp or and
    ret

k_mutex_lock:
    mov edx, [esp + 4] ;lock_addr
    .retry
    mov eax, 1
    xchg [edx], eax
    and eax, eax
    jnz .retry
    ret

k_mutex_unlock:
    mov edx, [esp + 4]
    mov dword [edx], 0
    ret


    