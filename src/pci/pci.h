#ifndef PCI_H
#define PCI_H

#include <stdint.h>

//어느 PCI장치의 어떤 설정 레지스터를 접근할지 결정하는 포트
#define PCI_CONFIG_ADDRESS 0xCF8

//실제로 설정 레지스터의 값을 읽고 쓰는 포트
#define PCI_CONFIG_DATA 0xCFC

struct pci_device 
{
  uint32_t bus; //메인보드 아파트안에 사는 동
  uint32_t device; // 아파트 안에 있는 호수 (101호)
  uint32_t sub_device; // (101)호수안에 사는 사람들... 
  uint32_t vendor_id; //제조회사
  uint32_t device_id; //장치명
  uint32_t port_addr; // (101호) 직통 전화번호...
  uint32_t irq; //벨소리번호
};

uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);
void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void pci_scan_bus();
struct pci_device pci_get_device(uint16_t vendor_id, uint16_t device_id);
#endif
