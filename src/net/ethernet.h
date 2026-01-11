#ifndef __ETHERNET_H__
#define __ETHERNET_H__

#include <stdint.h>

#define ETH_MAC_LEN 6
#define ETH_IP_LEN 4
#define ETH_HEADER_LEN 14
#define ETH_DATA_LEN 1500
#define ETH_MIN_DATA_LEN 46

#define ETH_TYPE_IP 0x0800  //IP프로토콜
#define ETH_TYPE_ARP 0x0806 //ARP프로토콜

struct ethernet_header
{
    uint8_t dest[ETH_MAC_LEN];
    uint8_t sender[ETH_MAC_LEN];
    uint16_t type;
}__attribute__((packed));

int ethernet_send(uint32_t port_addr,uint8_t *data, uint8_t *dest, uint8_t *src, uint16_t type, uint32_t len);
int ethernet_receive(uint8_t *frame, uint32_t port_addr);

#endif