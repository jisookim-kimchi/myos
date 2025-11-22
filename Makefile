FILES = ./build/kernel.asm.o ./build/kernel.o ./build/idt/idt.asm.o \
		./build/memory/memory.o ./build/idt/idt.o ./build/io/io.asm.o \
		./build/memory/heap/heap.o ./build/memory/heap/kernel_heap.o \
		./build/memory/paging/paging.o ./build/memory/paging/paging.asm.o \
		./build/disk/disk.o ./build/disk/streamer.o ./build/filesystem/pathparser.o \
		./build/filesystem/file.o ./build/string/string.o ./build/filesystem/fat16.o \

INCLUDES = -I./src/
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-loops -falign-labels -fstrength-reduce -fomit-frame-pointer -fno-asynchronous-unwind-tables -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

#if : input file 
#of : output file
#dd : copy and convert a file
all: ./bin/boot.bin ./bin/kernel.bin
	dd if=./bin/boot.bin of=./bin/myos.bin
	dd if=./bin/kernel.bin >> ./bin/myos.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/myos.bin

	# mount the myos.bin file to /mnt/d
	sudo mount -t vfat ./bin/myos.bin /mnt/d
	#copy a file over
	sudo cp ./hello.txt /mnt/d/
	sudo umount /mnt/d

./bin/kernel.bin: $(FILES)
	i686-elf-ld -g -relocatable $(FILES) -o ./build/kernelfull.o
	i686-elf-gcc $(FLAGS) -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o

./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf -g ./src/kernel.asm -o ./build/kernel.asm.o

./build/kernel.o: ./src/kernel.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o

./build/idt/idt.asm.o: ./src/idt/idt.asm
	nasm -f elf -g ./src/idt/idt.asm -o ./build/idt/idt.asm.o

./build/idt/idt.o: ./src/idt/idt.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/idt -std=gnu99 -c ./src/idt/idt.c -o ./build/idt/idt.o

./build/memory/memory.o: ./src/memory/memory.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/memory -std=gnu99 -c ./src/memory/memory.c -o ./build/memory/memory.o

./build/io/io.asm.o: ./src/io/io.asm
	nasm -f elf -g ./src/io/io.asm -o ./build/io/io.asm.o

./build/memory/heap/heap.o: ./src/memory/heap/heap.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/memory/heap -std=gnu99 -c ./src/memory/heap/heap.c -o ./build/memory/heap/heap.o

./build/memory/heap/kernel_heap.o: ./src/memory/heap/kernel_heap.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/memory/heap -std=gnu99 -c ./src/memory/heap/kernel_heap.c -o ./build/memory/heap/kernel_heap.o

./build/memory/paging/paging.o: ./src/memory/paging/paging.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/memory/paging -std=gnu99 -c ./src/memory/paging/paging.c -o ./build/memory/paging/paging.o

./build/memory/paging/paging.asm.o: ./src/memory/paging/paging.asm
	nasm -f elf -g ./src/memory/paging/paging.asm -o ./build/memory/paging/paging.asm.o

./build/disk/disk.o: ./src/disk/disk.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/disk -std=gnu99 -c ./src/disk/disk.c -o ./build/disk/disk.o

./build/disk/streamer.o : ./src/disk/streamer.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/disk -std=gnu99 -c ./src/disk/streamer.c -o ./build/disk/streamer.o

./build/filesystem/pathparser.o : ./src/filesystem/pathparser.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/filesystem -std=gnu99 -c ./src/filesystem/pathparser.c -o ./build/filesystem/pathparser.o

./build/filesystem/file.o : ./src/filesystem/file.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/filesystem -std=gnu99 -c ./src/filesystem/file.c -o ./build/filesystem/file.o

./build/filesystem/fat16.o : ./src/filesystem/fat16.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/filesystem -std=gnu99 -c ./src/filesystem/fat16.c -o ./build/filesystem/fat16.o

./build/string/string.o : ./src/string/string.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I ./src/string -std=gnu99 -c ./src/string/string.c -o ./build/string/string.o

clean:
	rm -rf ./bin/boot.bin
	rm -rf ./bin/kernel.bin
	rm -rf ./bin/myos.bin
	rm -rf $(FILES)
	rm -rf ./build/kernelfull.o

re:
	make clean
	make all