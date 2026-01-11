#include "ethernet.h"
#include "../memory/memory.h"
#include "../rtl8139_driver/rtl8139.h"
#include "../bits.h"
uint8_t ether_frame[1514];

int ethernet_send(uint32_t port_addr,uint8_t *data, uint8_t *dest, uint8_t *src, uint16_t type, uint32_t len)
{
    if (len > ETH_DATA_LEN || !src || !dest || !data)
    {
        return -1;
    }
    uint8_t frame[ETH_HEADER_LEN + ETH_DATA_LEN];
    struct ethernet_header *header = (struct ethernet_header*)frame;

    ft_memcpy(header->dest, dest, ETH_MAC_LEN);
    ft_memcpy(header->sender, src, ETH_MAC_LEN);
    header->type = htons(type);
    ft_memcpy(frame + ETH_HEADER_LEN, data, len);
    rtl8139_packet_send(port_addr, frame, ETH_HEADER_LEN + len);
    return 0;
}

int ethernet_receive(uint8_t *frame, uint32_t port_addr)
{
    uint8_t *packet;
    int len = rtl8139_packet_receive(port_addr, &packet);
    
    struct ethernet_header *header = (struct ethernet_header*)packet;
    uint16_t type = ntohs(header->type);
    if (type == ETH_TYPE_IP)
    {
        //IP처리
    }
    else if (type == ETH_TYPE_ARP)
    {
        //ARP처리
    }
    else
    {
        return -1;
    }
    ft_memcpy(frame, packet, len);
    return len;
}