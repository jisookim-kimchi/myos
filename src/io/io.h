#ifndef IO_H
#define IO_H

unsigned char insb(unsigned short port); // read byte from port 1byte
unsigned short insw(unsigned short port); // read word from port 2bytes

void outsb(unsigned short port, unsigned char data);
void outsw(unsigned short port, unsigned short data);

#endif