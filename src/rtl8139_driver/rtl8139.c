#include "rtl8139.h"
#include "../io/io.h"
#include "../idt/idt.h"
#include "../memory/memory.h"
#include "../net/ethernet.h"
#include "../kernel_print.h"

static uint8_t rx_buffer[32768 + 16 + 1500] __attribute__((aligned(4))); // 받을 메세지 보관함 (32KB로 확장)
static uint8_t tx_buffer[4][1536] __attribute__((aligned(4))); //보낼 메세지 보관함.
static uint8_t tx_slot = 0;
static uint8_t my_mac[6];
static uint8_t my_ip[4];
static uint32_t rtl8139_port = 0;  // Store port for interrupt handler
static uint16_t rx_read_ptr = 0;   // Track our read position

uint8_t* rtl8139_get_ip()
{
    return my_ip;
}

uint8_t* rtl8139_get_mac()
{
    return my_mac;
}

void rtl8139_set_ip(uint8_t *ip)
{
    ft_memcpy(my_ip, ip, 4);
}

void rtl8139_init(uint32_t port_addr)
{
    rtl8139_port = port_addr;  // Save for interrupt handler
    outsb(port_addr + RTL8139_CONFIG1, 0x00); //RTL8139 power on (폰 전원 켜기)
    outsb(port_addr + RTL8139_CMD, 0x10); //폰 초기화.
    while ((insb(port_addr + RTL8139_CMD) & 0x10) != 0)//RTL 8139 폰 켜질때까지 기다려.
    {

    }
    rtl8139_read_mac(port_addr, my_mac);
    outl(port_addr + RTL8139_RBSTART, (uint32_t)rx_buffer); //전화 받으면 여기에 저장해, 전화 기록 설정.
    outsw(port_addr + RTL8139_CAPR, 0xFFF0); // Initialize read pointer (must be 0xFFF0)
    rx_read_ptr = 0;  // Our software read pointer starts at 0
    outsw(port_addr + RTL8139_IMR, 0x01 | 0x04); //전화 벨소리 울리거나(수신) 전화를 하면(송신), 인터럽트!설정!
    outl(port_addr + RTL8139_RCR, 0xf | (1 << 7) | (0x03 << 11)); // 모든 패킷 수신 + Wrap 모드 + 32KB 버퍼 설정
    outsb(port_addr + RTL8139_CMD, 0x0C); //이제 전화 수신 송신 가능!
}

void rtl8139_read_mac(uint32_t port_addr, uint8_t *mac)
{
    for (int i = 0; i < 6; i++)
    {
        mac[i] = insb(port_addr + i);
    }
}

void rtl8139_handler(struct interrupt_frame* frame)
{
    uint32_t port_addr = rtl8139_port;
    uint16_t status = insw(port_addr + RTL8139_ISR);
    if (status & 0x01)
    {
        uint8_t *packet_data;
        while (1) 
        {
            int length = rtl8139_packet_receive(port_addr, &packet_data);
            if (length <= 0)
                break;
            ethernet_receive(packet_data, port_addr, length);
        }
    }
    if (status & 0x04) //메세지 보내기완료.
    {
       print("[RTL8139] Packet sent :) \n");
    }
    outsw(port_addr + RTL8139_ISR, status);
}

void rtl8139_register_irq(uint8_t irq)
{
    idt_register_interrupt_callback(irq + 32, rtl8139_handler);
    
    if (irq >= 8)
    {
        uint8_t master_mask = insb(0x21);
        master_mask &= ~0x04;
        outsb(0x21, master_mask);
        uint8_t slave_mask = insb(0xA1);
        slave_mask &= ~(1 << (irq - 8));
        outsb(0xA1, slave_mask);
    }
    else
    {
        uint8_t mask = insb(0x21);
        mask &= ~(1 << irq);
        outsb(0x21, mask);
    }
}

/*
전체 이더넷 프레임 크기
= 헤더 + 데이터 + CRC(데이터 검증용.)
= 14 + 1500 + 4 = 1518
*/
int rtl8139_packet_receive(uint32_t port_addr, uint8_t **out_data)
{
    uint16_t cbr = insw(port_addr + 0x3A);  // Current Buffer Read (hardware)
    
    // if CBR == our read pointer, no new packets
    if (cbr == rx_read_ptr)
    {
        return -1;
    }
    
    uint16_t offset = rx_read_ptr % (32768 + 16);

    uint16_t status = *(uint16_t*)(rx_buffer + offset);

    if (!(status & 0x01))
    {
        return -1;
    }
    uint16_t length = *(uint16_t*)(rx_buffer + offset + 2);
    if (length > 1518 || length < 64)
    {
        return -1;
    }
    *out_data = rx_buffer + offset + 4;
    
    rx_read_ptr = (rx_read_ptr + length + 4 + 3) & ~0b11;
    rx_read_ptr %= (32768 + 16);
    
    outsw(port_addr + RTL8139_CAPR, (rx_read_ptr - 16) & 0xFFFF);
    return length;
}


int rtl8139_packet_send(uint32_t port_addr, void *data, uint32_t length)
{
    if (length > 1518 || data == NULL)
    {
        return -1;
    }
    ft_memcpy(tx_buffer[tx_slot], data, length);
    
    outl(port_addr + RTL8139_TSAD0 + (tx_slot * 4), (uint32_t)tx_buffer[tx_slot]);
    outl(port_addr + RTL8139_TSD0 + (tx_slot * 4), length);
    tx_slot = (tx_slot + 1) % 4; //next slot
    
    return length;
}