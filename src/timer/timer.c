#include "timer.h"
#include "../io/io.h"
#include "../kernel.h"
#include "../task/task.h"
#include "../task/process.h"

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
  idt_register_interrupt_callback(0x20, timer_handle_interrupt);
}

void timer_handle_interrupt(struct interrupt_frame *frame)
{
  tick++;

  task_run_scheduled_tasks(tick);

  // Round Robin Scheduler
  // Only switch if we have a current task (multitasking initialized)
  // and ONLY if the interrupt happened in user mode (Ring 3).
  // This prevents the scheduler from preempting the kernel during critical paths like syscalls.
  // Exception: If the task explicitly blocked itself (TASK_BLOCKED), we MUST switch content.
  if (get_cur_task() && ((frame->cs & 0x03) == 0x03 || get_cur_task()->state == TASK_BLOCKED))
  {
      struct task* current_task = get_cur_task();
      current_task->ticks_usage++;

      if (current_task->ticks_usage > 10)
      {
          if (current_task->priority < TASK_PRIORITY_LOW)
          {
              current_task->priority++;
          }
          current_task->ticks_usage = 0;
      }

      struct task* next_task = get_next_task();
      if (next_task && next_task != current_task)
      {
          next_task->ticks_usage = 0;
          save_registers(frame);
          task_switch(next_task);
          task_return(&next_task->regs);
      }
  }
  

  if (tick % 100 == 0)
  {
    struct task* t = get_cur_task();
    if (t)
    {
      // print("PID: ");
      // print_int(t->process->id);
      // print(" Pri: ");
      // print_int(t->priority);
      // print("\n");
    }
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