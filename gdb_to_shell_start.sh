#!/bin/bash

gdb -ex "set confirm off" \
    -ex "symbol-file ./build/kernel.elf" \
    -ex "add-symbol-file ./src/programs/shell/shell.bin 0x10000000" \
    -ex "target remote localhost:1234" \
    -ex "break main" \
    -ex "c"
