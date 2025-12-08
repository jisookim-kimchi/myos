BUILD_DIR := ./build
BIN_DIR := ./bin
SRC_DIR := ./src

FILES = $(BUILD_DIR)/kernel.asm.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/idt/idt.asm.o \
		$(BUILD_DIR)/memory/memory.o $(BUILD_DIR)/idt/idt.o $(BUILD_DIR)/io/io.asm.o \
		$(BUILD_DIR)/memory/heap/heap.o $(BUILD_DIR)/memory/heap/kernel_heap.o \
		$(BUILD_DIR)/memory/paging/paging.o $(BUILD_DIR)/memory/paging/paging.asm.o \
		$(BUILD_DIR)/disk/disk.o $(BUILD_DIR)/disk/streamer.o $(BUILD_DIR)/filesystem/pathparser.o \
		$(BUILD_DIR)/filesystem/file.o $(BUILD_DIR)/string/string.o $(BUILD_DIR)/filesystem/fat16.o \
		$(BUILD_DIR)/gdt/gdt.o $(BUILD_DIR)/gdt/gdt.asm.o \
		$(BUILD_DIR)/task/tss.asm.o \
		$(BUILD_DIR)/task/task.o \
		$(BUILD_DIR)/task/process.o \
		$(BUILD_DIR)/task/task.asm.o \
		$(BUILD_DIR)/isr80h/isr80h.o \
		$(BUILD_DIR)/isr80h/misc.o

INCLUDES = -I$(SRC_DIR)/
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-loops -falign-labels -fstrength-reduce -fomit-frame-pointer -fno-asynchronous-unwind-tables -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc
	
.PHONY: all clean

all: $(BIN_DIR)/boot.bin $(BIN_DIR)/kernel.bin $(BIN_DIR)/myos.bin programs

$(BIN_DIR)/myos.bin: $(BIN_DIR)/boot.bin $(BIN_DIR)/kernel.bin programs
	rm -rf $(BIN_DIR)/myos.bin
	dd if=$(BIN_DIR)/boot.bin of=$(BIN_DIR)/myos.bin
	dd if=$(BIN_DIR)/kernel.bin >> $(BIN_DIR)/myos.bin
	dd if=/dev/zero bs=1048576 count=16 >> $(BIN_DIR)/myos.bin
	sudo mount -t vfat $(BIN_DIR)/myos.bin /mnt/d
	sudo cp ./test.txt /mnt/d
	sudo cp ./src/programs/blank/blank.bin /mnt/d
	sudo umount /mnt/d

$(BIN_DIR)/kernel.bin: $(FILES)
	i686-elf-ld -g -relocatable $(FILES) -o $(BUILD_DIR)/kernelfull.o
	i686-elf-gcc $(FLAGS) -T $(SRC_DIR)/linker.ld -o $(BIN_DIR)/kernel.bin -ffreestanding -O0 -nostdlib $(BUILD_DIR)/kernelfull.o

$(BIN_DIR)/boot.bin: $(SRC_DIR)/boot/boot.asm
	mkdir -p $(BIN_DIR)
	nasm -f bin $(SRC_DIR)/boot/boot.asm -o $(BIN_DIR)/boot.bin

$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	mkdir -p $(dir $@)
	nasm -f elf -g $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I $(dir $<) -std=gnu99 -c $< -o $@

programs:
	cd ./src/programs/blank && $(MAKE) all

programs_clean:
	cd ./src/programs/blank && $(MAKE) clean

clean: programs_clean
	rm -rf $(BIN_DIR)/*
	rm -rf $(BUILD_DIR)/*