#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

// Forward declaration
struct interrupt_frame;

/***********************
    Register Offsets
***********************/

// MAC Address (6 bytes)
#define RTL8139_MAC0 0x00 // MAC 주소 바이트 0-5

// TX Status Descriptor : packet status
// TX Start Address : packet address, where to find the packet 
#define RTL8139_TSD0 0x10 // TX Status Descriptor 0
#define RTL8139_TSD1 0x14
#define RTL8139_TSD2 0x18
#define RTL8139_TSD3 0x1C
#define RTL8139_TSAD0 0x20 // TX Start Address 0
#define RTL8139_TSAD1 0x24
#define RTL8139_TSAD2 0x28
#define RTL8139_TSAD3 0x2C

// packet structure : 
/*
    [4bytes header] [packet data(maximum. 1500bytes)]
           |
    status + length    
*/
// Receive Buffer Address 8kb(it include header,header is 4 bytes) + buffer(16bytes);
// normally ethernet packet max size is 1500 bytes, it calls MTU.
// wrap : at 8192 starting packet then 8192 + 1500 it is over 8192 so it wrap to 1
// so we gonna  8192 + 16(hw padding) + 1500(overflow revention)


#define RTL8139_RBSTART 0x30

// Command Register
#define RTL8139_CMD 0x37
#define RTL8139_CMD_RESET 0x10 // Reset bit
#define RTL8139_CMD_RE 0x08    // Receiver Enable
#define RTL8139_CMD_TE 0x04    // Transmitter Enable

// Interrupt Registers
// iMR: interrupt mask register, which interrupts to enable
// iSR: interrupt status register, which interrupts are pending
#define RTL8139_IMR 0x3C
#define RTL8139_ISR 0x3E

// RX Configuration Register
// RCR: receive configuration register, which receive mode to enable,
// ex) Broadcast ? or only my MAC? or ALL?
#define RTL8139_RCR 0x44

// Config Register (Power on)
// 0x00 : Power on
#define RTL8139_CONFIG1 0x52

// CAPR (Current Address of Packet Read)
// until where the packet is read
#define RTL8139_CAPR 0x38

struct rtl8139_device
{

};

void rtl8139_init(uint32_t port_addr);
void rtl8139_read_mac(uint32_t port_addr, uint8_t *mac);
void rtl8139_handler(struct interrupt_frame* frame); 
void rtl8139_register_irq(uint8_t irq);
int rtl8139_packet_receive(uint32_t port_addr, uint8_t **out_data);
int rtl8139_packet_send(uint32_t port_addr, void *data, uint32_t length);

uint8_t* rtl8139_get_mac();
uint8_t* rtl8139_get_ip();
void rtl8139_set_ip(uint8_t *ip);
#endif