#ifndef __BITS_H__
#define __BITS_H__

#include <stdint.h>

// network to host
// big endian to little endian (16bits)
// network to host short
static inline uint16_t ntohs(uint16_t netshort)
{
    return (netshort >> 8) | (netshort << 8);
}

// host to network
// little endian to big endian (16bits)
static inline uint16_t htons(uint16_t hostshort)
{
    return (hostshort >> 8) | (hostshort << 8);
}

// big endian to little endian (32bits)
// network to host long
static inline uint32_t ntohl(uint32_t netlong)
{
    return ((netlong >> 24) & 0xFF) |
           ((netlong >> 8) & 0xFF00) |
           ((netlong << 8) & 0xFF0000) |
           ((netlong << 24) & 0xFF000000);
}

#endif