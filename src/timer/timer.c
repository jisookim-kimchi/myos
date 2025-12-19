#include "timer.h"
#include "../io/io.h"
#include "../kernel.h"
#include "../task/task.h"

static uint32_t tick = 0;

/*
    (1.193182 MHz / freq)
    Command Port 0x43: 00110110b (Channel 0, Lo/Hi byte, Mode 3, Binary)
*/
void timer_init(int freq)
{

    int divisor = 1193180 / freq;
   
    outsb(0x43, 0x36);

    // Divisor 보내기 (Lo -> Hi 순서)
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

    outsb(0x40, l);
    outsb(0x40, h);
    idt_register_interrupt_callback(0x20, timer_tiktok);
}

void timer_tiktok(struct interrupt_frame *frame)
{
    tick++;
    // 나중에 여기에 "프로세스 스위칭" 코드가 들어갈 예정입니다.
    // 지금은 1초마다 점을 찍어서 살아있는지 확인해봅시다. (freq가 100일 때)
    task_run_scheduled_tasks(tick);
    if (tick % 100 == 0)
    {
        //print(".");
    }
}

uint32_t get_tick()
{
    return tick;
}

void sleep(uint32_t tick)
{
   task_sleep_until(tick * 100);
}