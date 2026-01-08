#include "kernel.h"
#include "idt/idt.h"
#include "disk/disk.h"
#include "keyboard/keyboard.h"
#include "memory/heap/kernel_heap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "config.h"
#include "gdt/gdt.h"
#include "task/tss.h"
#include "isr80h/isr80h.h"
#include "kernel_print.h"
#include "string/string.h"
#include "task/process.h"
#include "timer/timer.h"

void __attribute__((section(".entry"))) start(void)
{
  paging_switch_to_kernel();
}

struct tss tss;
struct gdt gdt_real[MYOS_TOTAL_GDT_SEGMENTS];
struct kernel_gdt gdt_structured[MYOS_TOTAL_GDT_SEGMENTS] =
{
    {.base = 0x00, .limit = 0x00, .type = 0x00},       // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a}, // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92}, // Kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xfa}, // User code segment (Read/Execute)
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2}, // User data segment
    {.base = 0x00, .limit = sizeof(tss), .type = 0xE9} // TSS Segment
};

void kernel_main()
{
  init_terminal();

  // [중요] GDT에 TSS의 실제 주소를 설정합니다.
  // 컴파일 시점에는 tss 변수의 위치를 모르기 때문에 실행 시점에 넣어줘야
  // 합니다.
  gdt_structured[5].base = (uint32_t)&tss;

  ft_memset(gdt_real, 0x00, sizeof(gdt_real));
  kernel_gdt_to_cpu_gdt(gdt_real, gdt_structured, MYOS_TOTAL_GDT_SEGMENTS);

  // Load the gdt
  gdt_load(gdt_real, sizeof(gdt_real));

  kernel_heap_init();
  idt_init();
  keyboard_init();
  timer_init(100);
  ft_memset(&tss, 0x00, sizeof(tss));

  // [TSS 설정: 커널의 안전 가옥(Safe House) 지정]
  tss.esp0 = 0x600000;
  tss.ss0 = MYOS_KERNEL_DATA_SELECTOR;
  tss_load(0x28);

  file_system_init();

  disk_search_and_init();

  // --- FAT16 Write Test ---
  int fd = fopen("0:/test2.txt", "w");
  if (fd <= 0)
  {
    print("Failed to open file for writing\n");
  }
  else
  {
    char *data = "Hello world!\n";
    int written = fwrite(data, 1, ft_strlen(data), fd);
    print("Written bytes: ");
    print_int(written);
    print("\n");
    fclose(fd);
    print("File closed.\n");
  }

  paging_init_kernel_4gb(PAGING_PRESENT | PAGING_WRITEABLE | PAGING_USER_ACCESS);

  paging_switch_to_kernel();

  enable_paging();

  isr80h_register_command_call();

  struct process *process = 0;
  int res = process_load("0:/shell.bin", &process);
  if (res < 0)
  {
    panic("process_load failed!\n");
  }

  enable_interrupts();
  task_run_first_ever_task();
  while (1)
  {

  }
}