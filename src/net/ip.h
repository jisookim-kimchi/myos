#ifndef IP_H

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

#include <stdint.h>

//IP 헤더 구조체
//dscp_ecn : 품질 혼잡  dscp : 패킷 우선순위, ECN : 네트워크 막힘 알림, ping 에선 그냥 0.
//identification = packet id
//header checksum : 헤더 오류검사.  0xffff ok, 0x0000 error
//flags_fragment_offset : 프래그먼트(패킷의 조각) 오프셋

struct ip_header
{
    uint8_t version_and_header_len;     //ip버전(4)+ ip 헤더 길이(4)
    uint8_t dscp_ecn;                   //dscp(6) + ecn(2)
    uint16_t total_length;              //ip 헤더 + 데이터의 전체 길이
    uint16_t packet_id;                 // identification
    uint16_t flags_fragment_offset;     // 프래그먼트 오프셋(13) + 프래그먼트 플래그(3)
    uint8_t ttl;                        // Time To Live
    uint8_t protocol;                   // 프로토콜 
    uint16_t header_checksum;           // 헤더 체크섬
    uint32_t src;                       // 소스 IP
    uint32_t dst;                       // 목적지 IP
} __attribute__((packed));

void ip_send(uint32_t port_addr, uint8_t *data, uint32_t len, uint32_t dest, uint32_t protocol);
void ip_receive(uint32_t port_addr, uint8_t *data, int len);
uint16_t checksum(void *data, int len);
#endif