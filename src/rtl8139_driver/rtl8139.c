#include "rtl8139.h"
#include "../io/io.h"
#include "../idt/idt.h"
#include "../memory/memory.h"

static uint8_t rx_buffer[8192 + 16 + 1500] __attribute__((aligned(4))); // 받을 메세지 보관함.
static uint8_t tx_buffer[4][1536] __attribute__((aligned(4))); //보낼 메세지 보관함.
static uint8_t tx_slot = 0;
static uint8_t my_mac[6];
static uint8_t my_ip[4];

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
    outsb(port_addr + RTL8139_CONFIG1, 0x00); //RTL8139 power on (폰 전원 켜기)
    outsb(port_addr + RTL8139_CMD, 0x10); //폰 초기화.
    while ((insb(port_addr + RTL8139_CMD) & 0x10) != 0)//RTL 8139 폰 켜질때까지 기다려.
    {

    }
    rtl8139_read_mac(port_addr, my_mac);
    outl(port_addr + RTL8139_RBSTART, (uint32_t)rx_buffer); //전화 받으면 여기에 저장해, 전화 기록 설정.
    outsw(port_addr + RTL8139_IMR, 0x01 | 0x04); //전화 벨소리 울리거나(수신) 전화를 하면(송신), 인터럽트!설정!
    outl(port_addr + RTL8139_RCR, 0xf | (1 << 7)); //모든 call,문자 받기. + wrap 모드.
    outsb(port_addr + RTL8139_CMD, 0x0C); //이제 전화 수신 송신 가능!
}

void rtl8139_read_mac(uint32_t port_addr, uint8_t *mac)
{
    for (int i = 0; i < 6; i++)
    {
        mac[i] = insb(port_addr + i);
    }
}

void rtl8139_handler(uint32_t port_addr)
{
    uint16_t status = insw(port_addr + RTL8139_ISR);
    if (status & 0x01)
    {
        uint8_t *packet_data;
        int length = rtl8139_packet_receive(port_addr, &packet_data);
        if (length > 0)
        {
            //패킷처리.
        }
    }
    if (status & 0x04) //메세지 보내기완료.
    {
        // 전송 완료 처리.
    }
    outsw(port_addr + RTL8139_ISR, status);
}

void rtl8139_register_irq(uint8_t irq)
{
    idt_set(irq + 32, rtl8139_handler);
}

/*
전체 이더넷 프레임 크기
= 헤더 + 데이터 + CRC(데이터 검증용.)
= 14 + 1500 + 4 = 1518
*/
int rtl8139_packet_receive(uint32_t port_addr, uint8_t **out_data)
{
    uint16_t cur_ptr = insw(port_addr + RTL8139_CAPR);
    uint16_t offset = (cur_ptr + 16) % (8192 + 16);

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
    
    cur_ptr = (cur_ptr + length + 4 + 3) & ~0b11;  // 4바이트 정렬
    outsw(port_addr + RTL8139_CAPR, cur_ptr - 16);
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