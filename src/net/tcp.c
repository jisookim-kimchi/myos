#include "tcp.h"
#include "../bits.h"
#include "../memory/memory.h"
#include "ip.h"
#include "../rtl8139_driver/rtl8139.h" 
#include "../kernel_print.h"
#include "../timer/timer.h"
#include <stdbool.h>

static struct socket_cache socket_cache[MAX_SOCKET_ID];
static bool socket_used[MAX_SOCKET_ID] = {0};
static int socket_count = 0;
/*
    TCP checksum = ip tile + tcp
*/
uint16_t tcp_checksum(uint32_t src_ip, uint32_t dest_ip, struct tcp_header *header, uint8_t *data, int data_len)
{
    struct tcp_fake_header fake;
    fake.src_ip = src_ip;
    fake.dst_ip = dest_ip;
    fake.reserved = 0;
    fake.protocol = 6;
    fake.tcp_length = htons(sizeof(struct tcp_header) + data_len); //tcp header + datalen..

    header->checksum = 0;
    int total_len = sizeof(fake) + sizeof(struct tcp_header) + data_len;
    uint8_t buf[total_len];
    ft_memcpy(buf, &fake, sizeof(fake));
    ft_memcpy(buf + sizeof(fake), header, sizeof(struct tcp_header));
    ft_memcpy(buf + sizeof(fake) + sizeof(struct tcp_header), data, data_len);
    return checksum(buf, total_len);
}

void tcp_send(uint32_t port_addr, struct tcp_socket *socket, uint8_t flags, uint8_t *data, uint32_t len)
{
    uint32_t tcp_len = sizeof(struct tcp_header) + len;
    uint8_t packet[tcp_len];

    struct tcp_header *header = (struct tcp_header*)packet;
    ft_memset(header, 0, sizeof(struct tcp_header));

    header->src_port = htons(socket->src_port);
    header->dst_port = htons(socket->dst_port);
    header->sequence_number = htonl(socket->seq);
    header->acknowledgment_number = htonl(socket->ack);
    header->data_offset = 5;
    header->reserved = 0;
    header->flags = flags;
    header->window_size = htons(MAX_TCP_BUF_SIZE);
    header->checksum = 0;
    header->urgent_pointer = 0;

    if (len > 0)
    {
        ft_memcpy(packet + sizeof(struct tcp_header), data, len);
    }
    uint32_t src_ip = *(uint32_t*)rtl8139_get_ip();
    uint16_t checksum = tcp_checksum(src_ip, socket->dst_ip, header, data, len);
    header->checksum = checksum;

    ip_send(port_addr, packet, tcp_len, socket->dst_ip, IP_PROTO_TCP);
}

void tcp_receive(uint32_t port_addr, uint8_t *data, uint32_t len, uint32_t src_ip)
{
    struct tcp_header *tcp = (struct tcp_header*)data;
    uint32_t my_ip = *(uint32_t*)rtl8139_get_ip();
    uint8_t *payload = data + sizeof(struct tcp_header);
    int payload_len = len - sizeof(struct tcp_header);
    
    uint16_t sum = tcp_checksum(src_ip, my_ip, tcp, payload, payload_len);
    if (sum != 0 && sum != 0xFFFF)
    {
        print("TCP checksum error\n");
        return;
    }
    if (tcp->flags & TCP_SYN)
    {
        tcp_server_handler(port_addr, tcp, src_ip);
        return;
    }
    if (tcp->flags & TCP_FIN)
    {
        for (int i = 0; i < MAX_SOCKET_ID; i++)
        {
            if (socket_used[i] && 
                socket_cache[i].socket.dst_ip == src_ip && 
                socket_cache[i].socket.dst_port == ntohs(tcp->src_port))
            {
                // 우리가 FIN_WAIT 상태면 → CLOSED
                if (socket_cache[i].socket.state == TCP_FIN_WAIT)
                {
                    socket_cache[i].socket.state = TCP_CLOSED;
                    tcp_send(port_addr, &socket_cache[i].socket, TCP_ACK, NULL, 0);
                    socket_used[i] = false;
                    socket_count--;
                }
                // ESTABLISHED 상태면 → 서버가 먼저 종료
                else if (socket_cache[i].socket.state == TCP_ESTABLISHED)
                {
                    tcp_send(port_addr, &socket_cache[i].socket, TCP_ACK, NULL, 0);
                    socket_cache[i].socket.state = TCP_CLOSED;
                    socket_used[i] = false;
                    socket_count--;
                }   
                return;
            }
        }
    }

    if (tcp->flags & TCP_PSH)
    {
        // recv_buf에 저장
        // TODO: 구현
    }

    if (tcp->flags & TCP_ACK)
    {
        tcp_client_handler(port_addr, tcp, src_ip);
    }
}

// someone is asking for connection
// for security, use random initial seq number

void tcp_server_handler(uint32_t port_addr, struct tcp_header *header, uint32_t client_ip)
{
    struct tcp_socket socket;
    uint16_t client_port = ntohs(header->src_port);
    uint16_t server_port = ntohs(header->dst_port);
    uint32_t client_seq = ntohl(header->sequence_number);

    socket.src_ip = *(uint32_t*)rtl8139_get_ip();
    socket.dst_ip = client_ip;
    socket.src_port = server_port;
    socket.dst_port = client_port;
    socket.seq = get_tick() * 1000;
    socket.ack = client_seq + 1;
    socket.state = TCP_SYN_SENT;

    struct socket_cache *cache = &socket_cache[socket_count];
    ft_memcpy(&cache->socket, &socket, sizeof(struct tcp_socket));
    socket_count++;

    tcp_send(port_addr, &socket, TCP_SYN | TCP_ACK, NULL, 0);
}

// client want to connect to server 
void tcp_client_handler(uint32_t port_addr, struct tcp_header *header, uint32_t server_ip)
{
    print("CLIENT HANDLER\n");
    for (int i = 0; i < socket_count; i++)
    {
        struct socket_cache *cache = &socket_cache[i];
        if (cache->socket.dst_ip == server_ip && cache->socket.state == TCP_SYN_SENT)
        {
            uint32_t server_seq = ntohl(header->sequence_number);
            cache->socket.ack = server_seq + 1;
            cache->socket.state = TCP_ESTABLISHED;
            tcp_send(port_addr, &cache->socket, TCP_ACK, NULL, 0);
            return;
        }
    }
}


void tcp_test_connect(uint32_t port_addr, uint32_t dst_ip, uint16_t dst_port)
{
    struct socket_cache *cache = &socket_cache[socket_count++];
    
    cache->socket.src_ip = *(uint32_t*)rtl8139_get_ip();
    cache->socket.dst_ip = dst_ip;
    cache->socket.src_port = 12345;
    cache->socket.dst_port = dst_port;
    cache->socket.seq = get_tick() * 1000;
    cache->socket.ack = 0;
    cache->socket.state = TCP_SYN_SENT;
    tcp_send(port_addr, &cache->socket, TCP_SYN, NULL, 0);
}

int tcp_socket()
{
    for (int i = 0; i < MAX_SOCKET_ID; i++)
    {
        if (!socket_used[i])
        {
            socket_used[i] = true;
            socket_cache[i].socket.state = TCP_CLOSED;
            socket_count++;
            return i;
        }
    }
    return -1;
}

/*
    asking connection to server
    next_port : 
    49152 ~ 65535 : dynamic port
    1024 ~ 49151 : registered port
    to distinguish multi connections whe receiving replies from servers.
    each connection needs a unique src_ip, src_port, dst_ip, dst_port.
*/
int tcp_connect(int socketid, uint32_t port_addr, uint32_t dst_ip, uint16_t dst_port)
{
    if (socketid < 0 || socketid >= MAX_SOCKET_ID || !socket_used[socketid])
    {
        return -1;
    }
    
    struct socket_cache *cache = &socket_cache[socketid];
    static uint16_t next_port = 4915;

    cache->socket.src_ip = *(uint32_t*)rtl8139_get_ip();
    cache->socket.dst_ip = dst_ip;
    cache->socket.src_port = next_port++;
    if (next_port > 49152)
        next_port = 4915;

    cache->socket.dst_port = dst_port;
    cache->socket.seq = get_tick() * 1000;
    cache->socket.ack = 0;
    cache->socket.state = TCP_SYN_SENT;
    tcp_send(port_addr, &cache->socket, TCP_SYN, NULL, 0);
    return 0;
}

//write data to socket
int tcp_write(int socketid, uint32_t port_addr, uint8_t *data, uint32_t len)
{
    if (socketid < 0 || socketid >= MAX_SOCKET_ID || !socket_used[socketid])
    {
        return -1;
    }
    struct socket_cache *cache = &socket_cache[socketid];
    if (cache->socket.state != TCP_ESTABLISHED)
    {
        return -1;
    }
    tcp_send(port_addr, &cache->socket, TCP_PSH, data, len);
    cache->socket.seq += len;
    return len;
}

int tcp_close(int socketid, uint32_t port_addr)
{
    if (socketid < 0 || socketid >= MAX_SOCKET_ID || !socket_used[socketid])
    {
        return -1;
    }
    
    struct socket_cache *cache = &socket_cache[socketid];
    if (cache->socket.state != TCP_ESTABLISHED)
    {
        return -1;
    }
    cache->socket.state = TCP_FIN_WAIT;
    tcp_send(port_addr, &cache->socket, TCP_FIN | TCP_ACK, NULL, 0);
    return 0;
}
