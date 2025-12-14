#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEYBOARD_BUFFER_SIZE 1024

struct keyboard
{
    char key_buffer[KEYBOARD_BUFFER_SIZE];
    int head;
    int tail;
};

void keyboard_init();
void keyboard_handle_interrupt();   
char keyboard_pop();
void keyboard_push(char c);

#endif