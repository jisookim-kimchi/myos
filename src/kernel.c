#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kernel_heap.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "config.h"
#include "gdt/gdt.h"
#include "task/tss.h"
#include "filesystem/pathparser.h"
#include "task/process.h"

uint16_t *video_memory = 0;
uint16_t terminal_row = 0;
uint16_t terminal_column = 0;

uint16_t terminal_make_char(char c, uint8_t color)
{
    return (uint16_t)c | (uint16_t)color << 8;
}

void terminal_putchar(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    video_memory[index] = terminal_make_char(c, color);
}

void terminal_write_char(char c, uint8_t color)
{
    if (c == '\n')
    {
        terminal_column = 0;
        terminal_row++;
        return;
    }
    terminal_putchar(c, color, terminal_column, terminal_row);
    terminal_column++;
    if (terminal_column >= VGA_WIDTH)
    {
        terminal_column = 0;
        terminal_row++;
    }
}

void    init_terminal()
{
    video_memory = (uint16_t *)VIDEO_MEMORY_ADDRESS;
    terminal_column = 0;
    terminal_row = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(' ', 0x00, x, y); // 검은 배경으로 초기화
        }
    }
}

void print(const char *str)
{
    size_t len = ft_strlen(str);
    for (size_t i = 0; i < len; i++)
    {
        terminal_write_char(str[i], 0x0F); // 하얀색 글자, 검은 배경
    }
}

void panic(const char* msg)
{
    print(msg);
    while(1) {}
}

struct tss tss;
struct gdt gdt_real[MYOS_TOTAL_GDT_SEGMENTS];
struct kernel_gdt gdt_structured[MYOS_TOTAL_GDT_SEGMENTS] = 
{
    {.base = 0x00, .limit = 0x00, .type = 0x00},                // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},           // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92},            // Kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},              // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},             // User data segment
    {.base = (uint32_t)&tss, .limit=sizeof(tss), .type = 0xE9}      // TSS Segment
};

//for paging test
static paging_4gb_chunk_t *chunk = 0;
void kernel_main()
{
    init_terminal(); // 화면을 회색 배경으로 초기화
    print("Hello, Kernel World!\n");
    
    ft_memset(gdt_real, 0x00, sizeof(gdt_real));
    kernel_gdt_to_cpu_gdt(gdt_real, gdt_structured, MYOS_TOTAL_GDT_SEGMENTS);

    // Load the gdt
    gdt_load(gdt_real, sizeof(gdt_real));

    kernel_heap_init();
    idt_init();

    ft_memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = MYOS_KERNEL_DATA_SELECTOR;

    // Load the TSS
    tss_load(0x28);

    file_system_init();

    disk_search_and_init();

  

    //malloc debug test.
    // void *ptr = kernel_malloc(50);
    // void *ptr2 = kernel_malloc(5000);
    // void *ptr3 = kernel_malloc(5600);
    // kernel_free(ptr);
    // void *ptr4 = kernel_malloc(50);
    // if (ptr || ptr2 || ptr3 || ptr4)
    // {
    //     print("Kernel malloc succeeded!\n");
    // }

    chunk = paging_new_4gb(PAGING_PRESENT | PAGING_WRITEABLE | PAGING_USER_ACCESS);
    
    paging_switch(chunk->directory_entry);
    
    //char *ptr = kernel_zero_alloc(4096);
    //paging_set(get_paging_4gb_dir(chunk), (void*)0x1000, (uint32_t)ptr | PAGING_USER_ACCESS | PAGING_PRESENT | PAGING_WRITEABLE);

    enable_paging();

    // char *ptr2 = (char*)0x1000;
    // ptr2[0] = 'A';
    // ptr2[1] = 'B';
    // print(ptr2);
    // print(ptr);

    //disk_read_block(get_disk(0), 20, 4, &buf);

    //enable the system interrupts;
    //enable_interrupts();
    
    //print("Interrupts enabled.\n");
    // path_root_t *root = parse_path("0:/bin/shell.exe", NULL);
    // if (root)
    // {
    //    print("Path parsed successfully!\n");
    // }
    // else
    // {
    //     print("Path parsing failed!\n");
    // }
    // int fd = fopen("0:/test.txt", "r");
    // char buffer[22] = {0};
    // if (fd)
    // {
    //     // struct file_stat stat;
    //     // if (fstat(fd, &stat) < 0)
    //     // {
    //     //     print("File stat failed!\n");
    //     //     goto out;
    //     // }
    //     // print("File open!\n");
    //     // int res = fseek(fd, 23, FILE_SEEK_SET);
    //     // if (res < 0)
    //     // {
    //     //     print("File seek failed!\n");
    //     //     goto out;
    //     // }
    //     fread(fd, buffer, 1, 22);
    //     print(buffer);
    // }
    // if (fd <= 0)
    // {
    //     print("File open failed!\n");
    // }
    // fclose(fd);
    // file_system_unresolve(get_disk(0));

    struct process *process = 0;
    int res = process_load("0:/blank.bin", &process);
    if (res < 0)
    {
        panic("process_load failed!\n");
    }
    
    task_run_first_ever_task();

    while (1)
    {

    }
}