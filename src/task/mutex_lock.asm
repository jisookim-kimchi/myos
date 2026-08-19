section .text

global atomic_exchange
;int a_lock = 0;
;int atomic_exchange(int *ptr, int value)
;ptr 

atomic_exchange:
    mov edx, [esp + 4]   
    mov eax, [esp + 8]
    xchg [edx], eax
    ret
