#include "icmp.h"
#include "../kernel_print.h"
#include "ip.h"

void icmp_receive(uint32_t port_addr, uint8_t *data, int len, uint32_t src_ip)
{
    struct icmp_header *header = (struct icmp_header*)data;
    
   uint16_t received_checksum = header->checksum;
   header->checksum = 0;
   uint16_t calculated = checksum(data, len);
   header->checksum = received_checksum;
   if (calculated != received_checksum)
   {
    print("ICMP checksum error\n");
    return;
   }
    
    if (header->type == ICMP_ECHO_REQUEST)
    {
        icmp_send(port_addr, data, len, src_ip, ICMP_ECHO_REPLY);
    }
}

void icmp_send(uint32_t port_addr, uint8_t *data, uint32_t len, uint32_t dest_ip, uint8_t type)
{
    struct icmp_header *header = (struct icmp_header*)data;
    header->type = type;
    header->checksum = 0;
    header->checksum = checksum(data, len);
    ip_send(port_addr, data, len, dest_ip, IP_PROTO_ICMP);
}