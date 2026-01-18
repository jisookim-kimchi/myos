#include "tcp.h"
#include "../bits.h"
#include "../kernel_print.h"
#include "../memory/memory.h"
#include "../rtl8139_driver/rtl8139.h"
#include "../timer/timer.h"
#include "ip.h"
#include <stdbool.h>

struct socket_cache socket_cache[MAX_SOCKET_ID];
bool socket_used[MAX_SOCKET_ID] = {0};
static int socket_count = 0;
/*
    TCP checksum = ip tile + tcp
*/
uint16_t tcp_checksum(uint32_t src_ip, uint32_t dest_ip, struct tcp_header *header, uint8_t *data, int data_len)
{
  struct tcp_fake_header fake;

  uint32_t tcp_header_len = header->data_offset * 4;
  if (tcp_header_len < 20)
    tcp_header_len = 20;

  fake.src_ip = src_ip;
  fake.dst_ip = dest_ip;
  fake.reserved = 0;
  fake.protocol = 6;
  fake.tcp_length = htons(tcp_header_len + data_len);

  uint16_t original_checksum = header->checksum;
  header->checksum = 0;

  int total_len = sizeof(fake) + tcp_header_len + data_len;
  uint8_t buf[total_len];
  ft_memcpy(buf, &fake, sizeof(fake));
  ft_memcpy(buf + sizeof(fake), header, tcp_header_len);
  ft_memcpy(buf + sizeof(fake) + tcp_header_len, data, data_len);
  
  uint16_t result = checksum(buf, total_len);
  header->checksum = original_checksum;
  
  return result;
}

void tcp_send(uint32_t port_addr, struct tcp_socket *socket, uint8_t flags, uint8_t *data, uint32_t len)
{
  uint32_t tcp_len = sizeof(struct tcp_header) + len;
  uint8_t packet[tcp_len];

  struct tcp_header *header = (struct tcp_header *)packet;
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
  uint32_t src_ip = *(uint32_t *)rtl8139_get_ip();
  uint32_t dst_ip = socket->dst_ip;
  uint16_t checksum = tcp_checksum(src_ip, dst_ip, header, data, len);
  header->checksum = checksum;

  ip_send(port_addr, packet, tcp_len, dst_ip, IP_PROTO_TCP);
}

void tcp_receive(uint32_t port_addr, uint8_t *data, uint32_t len, uint32_t src_ip)
{
  struct tcp_header *tcp = (struct tcp_header *)data;
  uint32_t my_ip = *(uint32_t *)rtl8139_get_ip();
  uint32_t header_len = tcp->data_offset * 4;
  uint8_t *payload = data + header_len;
  int payload_len = len - header_len;

  uint16_t received_checksum = tcp->checksum;
  tcp->checksum = 0;
  uint16_t calculated_checksum = tcp_checksum(src_ip, my_ip, tcp, (uint8_t *)payload, payload_len);
  tcp->checksum = received_checksum;

  if (calculated_checksum != received_checksum)
  {
    print("TCP checksum error\n");
    return;
  }


  struct tcp_socket *socket = NULL;
  for (int i = 0; i < MAX_SOCKET_ID; i++)
  {
    if (socket_used[i] && socket_cache[i].socket.dst_ip == src_ip &&
        socket_cache[i].socket.dst_port == ntohs(tcp->src_port) &&
        socket_cache[i].socket.src_port == ntohs(tcp->dst_port))
    {
      socket = &socket_cache[i].socket;
      break;
    }
  }

  if ((tcp->flags & TCP_SYN) && (tcp->flags & TCP_ACK))
  {
    tcp_client_handler(port_addr, tcp, src_ip);
    return;
  }

  if (tcp->flags & TCP_SYN)
  {
    tcp_server_handler(port_addr, tcp, src_ip);
    return;
  }
  if (!socket)
  {
    return;
  }

  if (tcp->flags & TCP_FIN)
  {
    // FIN은 sequence 번호를 1 소비합니다
    socket->ack = ntohl(tcp->sequence_number) + 1;
    
    // FIN에 대한 ACK 보내기
    tcp_send(port_addr, socket, TCP_ACK, NULL, 0);
    
    socket->state = TCP_CLOSED;
    
    // 소켓 해제
    for (int i = 0; i < MAX_SOCKET_ID; i++)
    {
      if (&socket_cache[i].socket == socket)
      {
        socket_used[i] = false;
        socket_count--;
        break;
      }
    }
    return;
  }

  if (tcp->flags & TCP_PSH || payload_len > 0)
  {
    socket->ack = ntohl(tcp->sequence_number) + payload_len;
    if (payload_len > 0)
    {
       if (socket->recv_len + payload_len < MAX_TCP_BUF_SIZE)
       {
          ft_memcpy(socket->recv_buf + socket->recv_len, payload, payload_len);
          socket->recv_len += payload_len;
       }
       else
       {
          print("[TCP] Receive buffer full! Data dropped.\n");
       }
    }
    tcp_send(port_addr, socket, TCP_ACK, NULL, 0);
  }

  if (tcp->flags & TCP_ACK)
  {
    //print("[TCP] : ACK received\n");
  }
}

// someone is asking for connection
// for security, use random initial seq number

void tcp_server_handler(uint32_t port_addr, struct tcp_header *header, uint32_t client_ip)
{
  int index = -1;
  for (int i = 0; i < MAX_SOCKET_ID; i++)
  {
    if (!socket_used[i])
    {
      index = i;
      break;
    }
  }

  if (index == -1)
  {
    return;
  }

  struct tcp_socket socket_new;
  uint16_t client_port = ntohs(header->src_port);
  uint16_t server_port = ntohs(header->dst_port);
  uint32_t client_seq = ntohl(header->sequence_number);

  socket_new.src_ip = *(uint32_t *)rtl8139_get_ip();
  socket_new.dst_ip = client_ip;
  socket_new.src_port = server_port;
  socket_new.dst_port = client_port;
  socket_new.seq = get_tick() * 1000;
  socket_new.ack = client_seq + 1;
  socket_new.state = TCP_SYN_SENT;

  struct socket_cache *cache = &socket_cache[index];
  socket_used[index] = true;
  ft_memcpy(&cache->socket, &socket_new, sizeof(struct tcp_socket));
  socket_count++;

  tcp_send(port_addr, &cache->socket, TCP_SYN | TCP_ACK, NULL, 0);
}

// client want to connect to server
void tcp_client_handler(uint32_t port_addr, struct tcp_header *header, uint32_t server_ip)
{
  for (int i = 0; i < MAX_SOCKET_ID; i++)
  {
    if (!socket_used[i])
      continue;
    struct socket_cache *cache = &socket_cache[i];
    if (cache->socket.dst_ip == server_ip && (cache->socket.state == TCP_SYN_SENT || cache->socket.state == TCP_ESTABLISHED))
    {
      uint32_t server_seq = ntohl(header->sequence_number);
      cache->socket.ack = server_seq + 1;
      
      if (cache->socket.state == TCP_SYN_SENT)
      {
        cache->socket.seq++; // SYN flag consumes 1 sequence number
        cache->socket.state = TCP_ESTABLISHED;
      }
      
      tcp_send(port_addr, &cache->socket, TCP_ACK, NULL, 0);
      return;
    }
  }
}

void tcp_test_connect(uint32_t port_addr, uint32_t dst_ip, uint16_t dst_port)
{
  int index = socket_count++;
  socket_used[index] = true;
  struct socket_cache *cache = &socket_cache[index];

  cache->socket.src_ip = *(uint32_t *)rtl8139_get_ip();
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
      socket_cache[i].socket.recv_len = 0;
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

  cache->socket.src_ip = *(uint32_t *)rtl8139_get_ip();
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

// write data to socket
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
  tcp_send(port_addr, &cache->socket, TCP_PSH | TCP_ACK, data, len);
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
  cache->socket.seq += 1;
  return 0;
}

int tcp_read(int socketid, uint8_t *buf, uint32_t len)
{
  if (socketid < 0 || socketid >= MAX_SOCKET_ID || !socket_used[socketid])
  {
    return -1;
  }
  struct tcp_socket *socket = &socket_cache[socketid].socket;
  if (socket->recv_len == 0)
    return 0;
  uint32_t read_len;
  if (len < socket->recv_len)
  {
    read_len = len;
  }
  else
  {
    read_len = socket->recv_len;
  }
  ft_memcpy(buf, socket->recv_buf, read_len);
  uint32_t remaining = socket->recv_len - read_len;
  if (remaining > 0)
  {
    for (uint32_t i = 0; i < remaining; i++)
    {
       socket->recv_buf[i] = socket->recv_buf[i + read_len];
    }
  }
  socket->recv_len = remaining;
  return read_len;
}
