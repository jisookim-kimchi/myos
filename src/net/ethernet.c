#include "ethernet.h"
#include "../bits.h"
#include "../kernel_print.h"
#include "../memory/memory.h"
#include "../rtl8139_driver/rtl8139.h"
#include "ip.h"
#include "arp.h"

int ethernet_send(uint32_t port_addr, uint8_t *data, uint8_t *dest, uint8_t *src, uint16_t type, uint32_t len)
{
  if (len > ETH_DATA_LEN || !src || !dest || !data)
  {
    return -1;
  }
  uint8_t frame[ETH_HEADER_LEN + ETH_DATA_LEN];
  struct ethernet_header *header = (struct ethernet_header *)frame;

  ft_memcpy(header->dest, dest, ETH_MAC_LEN);
  ft_memcpy(header->sender, src, ETH_MAC_LEN);
  header->type = htons(type);
  ft_memcpy(frame + ETH_HEADER_LEN, data, len);
  rtl8139_packet_send(port_addr, frame, ETH_HEADER_LEN + len);
  return 0;
}

int ethernet_receive(uint8_t *frame, uint32_t port_addr, uint32_t len)
{
  // frame parameter already contains the packet data from rtl8139_handler!
  // Do NOT call rtl8139_packet_receive again!

  struct ethernet_header *header = (struct ethernet_header *)frame;
  uint16_t type = ntohs(header->type);

  if (type == ETH_TYPE_IP)
  {
    ip_receive(port_addr, frame + ETH_HEADER_LEN, len - ETH_HEADER_LEN);
  }
  else if (type == ETH_TYPE_ARP)
  {
    arp_receive(port_addr, frame + ETH_HEADER_LEN, len - ETH_HEADER_LEN);
  }
  else
  {
    return -1;
  }
  return 0;
}