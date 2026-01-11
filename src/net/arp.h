#ifndef __ARP_H__
#define __ARP_H__

#include "ethernet.h"

#define ARP_HW_ETHERNET 1
#define ARP_PROTO_IP 0x0800
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2

static const uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct arp_header
{
    uint16_t hardware_type;    // 1 = 이더넷
    uint16_t protocol_type;    // 0x0800 = IP IPv4
    uint8_t hardware_len;      // 6 (MAC 길이)
    uint8_t ip_len;            // 4 (IP 길이)
    uint16_t operation;        // 1=요청, 2=응답
    uint8_t sender_mac[6];     // 보내는 사람 MAC
    uint8_t sender_ip[4];      // 보내는 사람 IP
    uint8_t target_mac[6];     // 받는 사람 MAC
    uint8_t target_ip[4];      // 받는 사람 IP
} __attribute__((packed));

struct arp_entry
{
    uint32_t ip;
    uint8_t mac[6];
};

void arp_request(uint32_t port_addr,uint32_t target_ip);
void arp_reply(uint32_t port_addr,struct arp_header *request);
void arp_receive(uint32_t port_addr, uint8_t *data, int len);
uint8_t* arp_cache_lookup(uint32_t ip);
void arp_cache_add(uint32_t ip, uint8_t *mac);

#endif