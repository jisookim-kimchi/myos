#include "ip.h"
#include "../bits.h"
#include "../rtl8139_driver/rtl8139.h"
#include "../memory/memory.h"
#include "ethernet.h"
#include "icmp.h"
#include "tcp.h"
#include "arp.h"
#include "../kernel_print.h"

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
    header.header_checksum = checksum(&header, sizeof(struct ip_header));

    uint8_t frame_data[sizeof(struct ip_header) + len];
    ft_memcpy(frame_data, &header, sizeof(struct ip_header));
    ft_memcpy(frame_data + sizeof(struct ip_header), data, len);

    uint8_t *dest_mac = arp_cache_lookup(dest);
    if (!dest_mac)
    {
        dest_mac = (uint8_t*)broadcast;
        arp_request(port_addr, dest);
    }
    ethernet_send(port_addr, frame_data, dest_mac, rtl8139_get_mac(), ETH_TYPE_IP, sizeof(struct ip_header) + len);
}

// receiver should check like this... 
// if (data(header) + checksum = 1111.... or 0) ok, else error
// so we have to return reversed result.
uint16_t checksum(void *data, int len)
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
    uint16_t received_checksum = header->header_checksum;
    
    header->header_checksum = 0;
    uint16_t calculated = checksum(header, sizeof(struct ip_header));
    header->header_checksum = received_checksum;  // Restore original

    if (calculated != received_checksum)
    {
        print("IP checksum error\n");
        return;
    }

    uint16_t ip_total_len = ntohs(header->total_length);
    uint32_t ip_header_len = (header->version_and_header_len & 0x0F) * 4;
    uint32_t payload_len = ip_total_len - ip_header_len;

    if (header->protocol == IP_PROTO_ICMP)
    {
        uint8_t *icmp_data = data + ip_header_len;
        icmp_receive(port_addr, icmp_data, payload_len, header->src);
    }
    else if (header->protocol == IP_PROTO_TCP)
    {
        tcp_receive(port_addr, data + ip_header_len, payload_len, header->src);
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

