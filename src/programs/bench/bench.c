#include "bench.h"
#include "../stdlib/stdlib.h"

volatile int turn = 0;
uint32_t latency[SAMPLES];
int ping_count = 0;
volatile uint64_t start_tsc = 0;

void ping(void)
{
    for (volatile int i = 0; i < SAMPLES; i++)
    {
        start_tsc = read_tsc();
        turn = 1;
        task_wakeup((void*)&turn);
        while (turn != 0)
        {
            task_block((void*)&turn);
        }
    }
}

void pong(void)
{
    for (volatile int i = 0; i < SAMPLES; i++)
    {
        while (turn != 1)
        {
            task_block((void*)&turn);
        }
        uint64_t end_tsc = read_tsc();
        latency[ping_count++] = (uint32_t)(end_tsc - start_tsc);
        turn = 0;
        task_wakeup((void*)&turn);
    }
}
