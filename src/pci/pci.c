#include "pci.h"
#include "../io/io.h"
#include "../kernel_print.h"

// 1. PCI 장치에서 16비트(Word) 데이터를 읽어오는 함수
// bus: 버스 번호 (0~255)
// slot: 디바이스 번호 (0~31)
// func: 기능 번호 (0~7)
// offset: 레지스터 위치 (0, 2, 4...)
uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
  uint32_t addr = 0;
  uint32_t lbus  = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;
  uint16_t tmp = 0;
  addr = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
  
  outl(0xCF8, addr);

  //[device][vendor]
  //1. offset이 0이면 vendor id
  //2. offset이 2이면 device id
  tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
  return tmp;
}

//to get port address
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
  uint32_t addr = 0;
  uint32_t lbus  = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;
  addr = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
  
  outl(0xCF8, addr);

  //[device][vendor]
  //1. offset이 0이면 vendor id
  //2. offset이 2이면 device id
  return inl(0xCFC);
}

void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value)
{
  uint32_t addr = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
  outl(0xCF8, addr);

  uint32_t val = inl(0xCFC);
  uint32_t shift = (offset & 2) * 8;
  
  val &= ~(0xFFFF << shift); // 하위비트를 0000으로 0xaabb0000 이런식. 지웠으니까!
  val |= ((uint32_t)value << shift); //하위비트에 value를 넣음.
  
  outl(0xCFC, val);
}

void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
  uint32_t addr = 0;
  uint32_t lbus  = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;

  addr = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
  
  outl(0xCF8, addr);
  outl(0xCFC, value);
}

//출력용.
void pci_scan_bus()
{
  for (int bus = 0; bus < 256; bus++)
  {
    for (int slot = 0; slot < 32; slot++)
    {
        uint16_t vendor_id = pci_read_word(bus, slot, 0, 0);
        if (vendor_id != 0xFFFF)//if not empty!
        {
          uint16_t device_id = pci_read_word(bus, slot, 0, 2);
          print("Found PCI Device Bus: ");
          print_int(bus);
          print(", Slot: ");
          print_int(slot);
          print(", Vendor: ");
          print_hex(vendor_id);
          print(", Device: ");
          print_hex(device_id);
          print("\n");
        }
    }
  }
}

//0000 0100  : DMA(Direct Memory Access) enable - Lan card can access RAM directly without using CPU
static void pci_enable_bus_master(uint8_t bus, uint8_t slot, uint8_t func)
{
  uint16_t command = pci_read_word(bus, slot, func, 0x04);
  // print("PCI: Original command = 0x");
  // print_hex(command);
  // print("\n");
    
  command |= 0x04;   // Enable Bus Master (DMA)
  command &= ~0x400; // Clear bit 10 (Interrupt Disable) to ENABLE interrupts
  
  pci_write_word(bus, slot, func, 0x04, command);
    
  //uint16_t new_command = pci_read_word(bus, slot, func, 0x04);
  // print("PCI: new command = 0x");
  // print_hex(new_command);
  // print("\n");
}

struct pci_device pci_get_device(uint16_t vendor_id, uint16_t device_id)
{
  struct pci_device device = {0};
  for (int bus = 0; bus < 256; bus++)
  {
    for (int slot = 0; slot < 32; slot++)
    {
      if (pci_read_word(bus, slot, 0, 0) == vendor_id && pci_read_word(bus, slot, 0, 2) == device_id)
      {
        device.bus = bus;
        device.device = slot;
        device.sub_device = 0;
        device.vendor_id = vendor_id;
        device.device_id = device_id;
        uint32_t port_addr = pci_read_dword(bus, slot, 0, 0x10); //BAR0 : Base Address Register 0, 101호 직통번호..
        // print("PCI: BAR0 raw = 0x");
        // print_hex(port_addr);
        device.port_addr = port_addr & 0xFFFFFFFC;
        // print(" processed = 0x");
        // print_hex(device.port_addr);
        // print("\n");
        device.irq = pci_read_word(bus, slot, 0, 0x3C) & 0xFF;
        pci_enable_bus_master(device.bus, device.device, device.sub_device);
        return device;
      }
    }
  }
  return device;
}

