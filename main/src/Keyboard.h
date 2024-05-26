
// Written by Lynton "Pionwave" Schneider, 2024

#ifndef __PULSE_KEYBOARD_H__
#define __PULSE_KEYBOARD_H__


extern volatile char keyb[256];

void init_keyboard();
void shutdown_keyboard();


#endif