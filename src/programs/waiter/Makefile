all:
	mkdir -p ./build
	nasm -f elf ../stdlib/start.asm -o ./build/start.o
	nasm -f elf ../stdlib/stdlib.asm -o ./build/stdlib.asm.o
	i686-elf-gcc -I./src/ -I../stdlib/ -g -ffreestanding -falign-jumps -falign-functions -falign-loops -falign-labels -fstrength-reduce -fomit-frame-pointer -fno-asynchronous-unwind-tables -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -std=gnu99 -c ./waiter.c -o ./build/waiter.o
	i686-elf-gcc -I./src/ -I../stdlib/ -g -ffreestanding -falign-jumps -falign-functions -falign-loops -falign-labels -fstrength-reduce -fomit-frame-pointer -fno-asynchronous-unwind-tables -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -std=gnu99 -c ../stdlib/stdlib.c -o ./build/stdlib.o
	i686-elf-gcc -I./src/ -I../stdlib/ -g -ffreestanding -falign-jumps -falign-functions -falign-loops -falign-labels -fstrength-reduce -fomit-frame-pointer -fno-asynchronous-unwind-tables -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -std=gnu99 -c ../stdlib/malloc.c -o ./build/malloc.o
	i686-elf-ld -g -relocatable -o ./build/waiter_full.o ./build/start.o ./build/waiter.o ./build/stdlib.o ./build/malloc.o ./build/stdlib.asm.o
	i686-elf-gcc -g -T ./linker.ld -o ./waiter.bin -ffreestanding -O0 -nostdlib -fpic -g ./build/waiter_full.o

clean:
	rm -rf ./build/*
	rm -rf ./waiter.bin
