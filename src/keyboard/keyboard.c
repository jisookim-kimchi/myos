#include "keyboard.h"
#include "../io/io.h"
#include "../isr80h/io.h"
#include "../kernel.h"  
#include "../idt/idt.h"
#include "../task/task.h"
#include "../task/process.h"

void keyboard_init()
{
    // Process creation will handle initialization
}

static char scancode_to_ascii[] = 
{
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//Circular Queue
void keyboard_push(char c)
{
    struct process* process = get_cur_task()->process;
    if (!process)
    {
        return;
    }

    // head는 "다음에 쓰여질 칸의 번호(Index)"입니다.
    int next_head = (process->keyboard.head + 1) % KEYBOARD_BUFFER_SIZE;
    
    // 꽉 차면(Tail을 따라잡으면) 무시
    if (next_head != process->keyboard.tail)
    {
        process->keyboard.key_buffer[process->keyboard.head] = c; // 현재 head 위치에 저장
        process->keyboard.head = next_head; // head 한 칸 전진
    }
    task_wakeup(&process->keyboard);
}

char keyboard_pop()
{
    if (!get_cur_task())
    {
        return 0;
    }

    struct process* process = get_cur_task()->process;
    if (process->keyboard.head == process->keyboard.tail)
    {
        return 0; 
    }
    
    char c = process->keyboard.key_buffer[process->keyboard.tail];
    process->keyboard.tail = (process->keyboard.tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

/*
    0x60 : Keyboard interrupt,  
    0x80 : Keyboard release,
    if we get 0x80, it means that the key is released.
    so if we get scancode then we push into Queue
*/
void keyboard_handle_interrupt()
{
    uint8_t scancode = insb(0x60); 
    
    if (scancode & 0x80)
    {
        return;
    }

    if (scancode < sizeof(scancode_to_ascii))
    {
        char c = scancode_to_ascii[scancode];
        if (c != 0)
        {
            keyboard_push(c);
        }
    }
}