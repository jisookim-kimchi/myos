#ifndef TCP_H
#define TCP_H
#define MAX_SOCKET_CACHE 10

#include <stdint.h>

/*
    FIN : connection close
    SYN : connection request
    RST : connection reset
    PSH : immediately push
    ACK : acknowledgment
*/
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

#define MAX_TCP_BUF_SIZE 4096

/*
    sequence number :  order number
    acknowledgment number : next expected number(acknowledgment)
    data offset : size of header
    reserved : 
    flags : TCP_FIN, TCP_SYN, TCP_RST, TCP_PSH, TCP_ACK
    window_size : window size(maximum amount of data that can be sent)
    checksum : checksum
    urgent_pointer : urgent pointer
    data offset and reserved share the same byte so total 1 byte
    and flags share the same byte so total 2 bytes
*/

enum tcp_state
{
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT
};

struct tcp_header
{
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint8_t data_offset:4;
    uint8_t reserved:4;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} __attribute__((packed));

struct tcp_fake_header
{
    uint32_t src_ip;      // 출발 IP (4바이트)
    uint32_t dst_ip;      // 도착 IP (4바이트)
    uint8_t reserved;     // 0 (1바이트)
    uint8_t protocol;     // 6 = TCP (1바이트)
    uint16_t tcp_length;  // TCP 헤더+데이터 길이 (2바이트)
} __attribute__((packed));

struct tcp_socket
{
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t send_buf[MAX_TCP_BUF_SIZE];
    uint8_t recv_buf[MAX_TCP_BUF_SIZE];
    enum tcp_state state;
};

struct socket_cache
{
    struct tcp_socket socket;
};

uint16_t tcp_checksum(uint32_t src_ip, uint32_t dest_ip, struct tcp_header *header, uint8_t *data, int data_len);

void tcp_send(uint32_t port_addr, struct tcp_socket *socket, uint8_t flags, uint8_t *data, uint32_t len);
void tcp_receive(uint32_t port_addr, uint8_t *data, uint32_t len, uint32_t src_ip);
void tcp_server_handler(uint32_t port_addr, struct tcp_header *header, uint32_t client_ip);
void tcp_client_handler(uint32_t port_addr, struct tcp_header *header, uint32_t server_ip);
#endif