#!/bin/bash
gdb -ex "target remote localhost:1234" -ex "b *0x100000" -ex "c" ./build/kernel.elf
