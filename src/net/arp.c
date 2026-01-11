#include "ethernet.h"
#include "arp.h"
#include "../memory/memory.h"
#include "../rtl8139_driver/rtl8139.h"
#include "../bits.h"

#define ARP_CACHE_SIZE 16

static struct arp_entry arp_cache[ARP_CACHE_SIZE];
static int arp_cache_count = 0;

void arp_request(uint32_t port_addr,uint32_t target_ip)
{
    struct arp_header h_arp;

    h_arp.hardware_type = htons(ARP_HW_ETHERNET);
    h_arp.protocol_type = htons(ARP_PROTO_IP);
    h_arp.hardware_len = ETH_MAC_LEN;
    h_arp.ip_len = ETH_IP_LEN;
    h_arp.operation = htons(ARP_OP_REQUEST);
    ft_memcpy(h_arp.sender_mac, rtl8139_get_mac(), ETH_MAC_LEN);
    ft_memcpy(h_arp.sender_ip, rtl8139_get_ip(), ETH_IP_LEN);
    ft_memset(h_arp.target_mac, 0, ETH_MAC_LEN);
    ft_memcpy(h_arp.target_ip, &target_ip, ETH_IP_LEN);

    ethernet_send(port_addr, (uint8_t*)&h_arp, (uint8_t*)broadcast, rtl8139_get_mac(), ETH_TYPE_ARP, sizeof(struct arp_header));
}

void arp_receive(uint32_t port_addr, uint8_t *data, int len)
{
    struct arp_header *h_arp = (struct arp_header*)data;
    if (h_arp->operation == ARP_OP_REPLY)
    {
        uint32_t sender_ip;
        ft_memcpy(&sender_ip, h_arp->sender_ip, ETH_IP_LEN);
        arp_cache_add(sender_ip, h_arp->sender_mac);
    }
    else if (h_arp->operation == ARP_OP_REQUEST)
    {
        uint32_t target;
        ft_memcpy(&target, h_arp->target_ip, ETH_IP_LEN);

        uint32_t my_ip;
        ft_memcpy(&my_ip, rtl8139_get_ip(), ETH_IP_LEN);
        if (target == my_ip)
        {
            uint32_t sender_ip;
            ft_memcpy(&sender_ip, h_arp->sender_ip, ETH_IP_LEN);
            arp_cache_add(sender_ip, h_arp->sender_mac);
            arp_reply(port_addr, h_arp);
        }
    }
}

void arp_reply(uint32_t port_addr,struct arp_header *request)
{
    struct arp_header reply;
    reply.hardware_type = htons(ARP_HW_ETHERNET);
    reply.protocol_type = htons(ARP_PROTO_IP);
    reply.hardware_len = ETH_MAC_LEN;
    reply.ip_len = ETH_IP_LEN;
    reply.operation = htons(ARP_OP_REPLY);
    ft_memcpy(reply.sender_mac, rtl8139_get_mac(), ETH_MAC_LEN);
    ft_memcpy(reply.sender_ip, rtl8139_get_ip(), ETH_IP_LEN);
    ft_memcpy(reply.target_mac, request->sender_mac, ETH_MAC_LEN);
    ft_memcpy(reply.target_ip, request->sender_ip, ETH_IP_LEN);
    ethernet_send(port_addr, (uint8_t*)&reply, (uint8_t*)request->sender_mac, rtl8139_get_mac(), ETH_TYPE_ARP, sizeof(struct arp_header));
}

uint8_t* arp_cache_lookup(uint32_t ip)
{
    for (int i = 0; i < ARP_CACHE_SIZE; i++)
    {
        if (arp_cache[i].ip == ip)
        {
            return arp_cache[i].mac;
        }
    }
    return NULL;
}

void arp_cache_add(uint32_t ip, uint8_t *mac)
{
    for (int i = 0; i < ARP_CACHE_SIZE; i++)
    {
        if (arp_cache[i].ip == ip)
        {
            ft_memcpy(arp_cache[i].mac, mac, ETH_MAC_LEN);
            return;
        }
    }

    if (arp_cache_count < ARP_CACHE_SIZE)
    {
        arp_cache[arp_cache_count].ip = ip;
        ft_memcpy(arp_cache[arp_cache_count].mac, mac, ETH_MAC_LEN);
        arp_cache_count++;
    }
}