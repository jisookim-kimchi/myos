#include "memory/debug/cache_check.h"
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
#include "io/io.h"
#include "pci/pci.h"
#include "rtl8139_driver/rtl8139.h"
#include "net/arp.h"
#include "net/icmp.h"
#include "net/tcp.h"

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
  
  //for debugging set comment
  //timer_init(100); // alarm per 10ms

  /*
  //0x10EC 리얼텍 제조사 번호, 0x8139 8139모델번호.
  pci_scan_bus();
  struct pci_device device = pci_get_device(0x10EC, 0x8139);
  rtl8139_init(device.port_addr);

  // uint8_t *mac = rtl8139_get_mac();
  // print("MAC Address: ");
  // for (int i = 0; i < 6; i++)
  // {
  //   print_hex(mac[i]);
  //   print(" ");
  // }
  // print("\n");

  rtl8139_register_irq(device.irq);
  // print("IRQ : ");
  // print_int(device.irq);
  // print("\n");
  // uint8_t test_packet[64] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // 브로드캐스트
  // rtl8139_packet_send(device.port_addr, test_packet, 64);
  // print("packet sent\n");

  // IP 설정!
  uint8_t my_ip[] = {10, 0, 2, 15};  // QEMU user network
  rtl8139_set_ip(my_ip);
  uint8_t target_ip_bytes[] = {10, 0, 2, 2};  // 10.0.2.2 (QEMU 게이트웨이)
  uint32_t target_ip = *(uint32_t*)target_ip_bytes;
  arp_request(device.port_addr, target_ip);

  uint8_t ping_data[8];
  struct icmp_header *icmp = (struct icmp_header*)ping_data;
  icmp->type = 8;
  icmp->code = 0;
  icmp->id = 1;
  icmp->sequence = 1;
  icmp->checksum = 0;
  for (volatile int i = 0; i < 1000000; i++) { }
  icmp_send(device.port_addr, ping_data, 8, target_ip, ICMP_ECHO_REQUEST);
  print("Ping sent!\n");
  */

  enable_interrupts();

  /*
  int sock = tcp_socket();
  tcp_connect(sock, device.port_addr, target_ip, 80);

  for (volatile int i = 0; i < 30000000; i++) { }

  tcp_write(sock, device.port_addr, (uint8_t*)"GET / HTTP/1.1\r\nHost: 10.0.2.2\r\n\r\n", 35);

  for (volatile int i = 0; i < 50000000; i++)
  {
    uint8_t buffer[1024];
    int bytes = tcp_read(sock, buffer, sizeof(buffer));
    (void)bytes;
    if (bytes > 0)
    {
      for(int j=0; j < bytes; j++)
      {
        terminal_write_char(buffer[j], 0x01); 
      }
    }
  }
  tcp_close(sock, device.port_addr);
  */



  ft_memset(&tss, 0x00, sizeof(tss));
  // [TSS 설정: 커널의 안전 가옥(Safe House) 지정]
  tss.esp0 = 0x600000;
  tss.ss0 = MYOS_KERNEL_DATA_SELECTOR;
  tss_load(0x28); // TSS 셀렉터
  
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

  test_cache_speed();

  isr80h_register_command_call();

  struct process *process = 0;
  int res = process_load("0:/blank.bin", &process);
  //int res = process_load("0:/shell.bin", &process);
  if (res < 0)
  {
    panic("process_load failed!\n");
  }

//   enable_interrupts(); // Moved up
  task_run_first_ever_task();
  while (1)
  {

  }
}