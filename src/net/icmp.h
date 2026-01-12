#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>

#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8

/*
type : 8=요청, 0=응답
code : 세부요청 ping은 type 0과 8만 사용. 11은 타임아웃 3은 목적지 불가능 우리는 8과 0만 사용.
checksum : 헤더 오류검사
id : ping을 보낸 프로그램 ID
sequence : ping을 보낸 프로그램 내에서의 순서
*/
struct icmp_header
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed));

void icmp_receive(uint32_t port_addr, uint8_t *data, int len, uint32_t src_ip);
void icmp_send(uint32_t port_addr, uint8_t *data, uint32_t len, uint32_t dest_ip, uint8_t type);
#endif