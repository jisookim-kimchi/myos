section .asm

global read_tsc

read_tsc:
    rdtsc
    ;eax = low , edx = high, since rdtsc is 64-bit instruction
    ret