#ifndef TIMER_H
#define TIMER_H

#include "../idt/idt.h"
#include <stdint.h>

void timer_init(int freq);
void timer_tiktok(struct interrupt_frame *frame);
uint32_t get_tick();
void sleep(uint32_t tick);
#endif