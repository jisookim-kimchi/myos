#include "ip.h"
#include "../bits.h"
#include "../rtl8139_driver/rtl8139.h"
#include "../memory/memory.h"
#include "ethernet.h"
#include "../kernel_print.h"
#include "icmp.h"

void ip_send(uint32_t port_addr, uint8_t *data, uint32_t len, uint32_t dest, uint32_t protocol)
{
    struct ip_header header;
    header.version_and_header_len = 0x45;
    header.dscp_ecn = 0;
    header.total_length = htons(len + sizeof(struct ip_header));
    header.packet_id = 0;
    header.flags_fragment_offset = 0;
    header.ttl = 64;
    header.protocol = protocol;
    header.header_checksum = 0;
    ft_memcpy(&header.src, rtl8139_get_ip(), 4);
    ft_memcpy(&header.dst, &dest, 4);
    header.header_checksum = ip_checksum(&header, sizeof(struct ip_header));

    uint8_t frame[ETH_HEADER_LEN + sizeof(struct ip_header) + len];
    struct ethernet_header *eth_header = (struct ethernet_header*)frame;
    ft_memcpy(eth_header->dest, rtl8139_get_mac(), 6);
    ft_memcpy(eth_header->sender, rtl8139_get_mac(), 6);
    eth_header->type = htons(ETH_TYPE_IP);
    ft_memcpy(frame + ETH_HEADER_LEN, &header, sizeof(struct ip_header));
    ft_memcpy(frame + ETH_HEADER_LEN + sizeof(struct ip_header), data, len);
    rtl8139_packet_send(port_addr, frame, ETH_HEADER_LEN + sizeof(struct ip_header) + len);
}

// receiver should check like this... 
// if (data(header) + checksum = 1111....) ok, else error
// so we have to return reversed result.
uint16_t ip_checksum(void *data, int len)
{
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t*)data;
    while (len > 1)
    {
        sum += *ptr++;
        len -= 2;
    }
    if (len == 1)
    {
        sum += *(uint8_t*)ptr;
    }
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return ~sum;
}

void ip_receive(uint32_t port_addr, uint8_t *data, int len)
{
    struct ip_header *header = (struct ip_header*)data;
    if (header->header_checksum != ip_checksum(header, sizeof(struct ip_header)))
    {
        print("IP checksum error\n");
        return;
    }
    if (header->protocol == IP_PROTO_ICMP)
    {
        uint8_t *icmp_data = data + sizeof(struct ip_header);
        int icmp_data_len = len - sizeof(struct ip_header);
        icmp_receive(port_addr, icmp_data, icmp_data_len, header->src);
    }
    else if (header->protocol == IP_PROTO_TCP)
    {
        //tcp 처리
    }
    else if (header->protocol == IP_PROTO_UDP)
    {
        //udp 처리
    }
    else
    {
        return;
    }
}

