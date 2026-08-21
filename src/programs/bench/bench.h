#ifndef BENCH_H
#define BENCH_H

#include <stdint.h>

#define SAMPLES 1000

extern uint32_t latency[SAMPLES];
extern int ping_count;

void ping(void);
void pong(void);

#endif
