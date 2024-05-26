
// Written by Lynton "Pionwave" Schneider, 2024

#ifndef __PULSE_TIMER_H__
#define __PULSE_TIMER_H__

extern volatile unsigned long tick_count;

void initialize_timer();
void restore_timer();

#endif
