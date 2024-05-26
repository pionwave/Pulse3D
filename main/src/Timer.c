
// Written by Lynton "Pionwave" Schneider, 2024

#include <dpmi.h>
#include <dos.h>
#include <go32.h>

#include "Timer.h"

volatile unsigned long tick_count = 0;
#define TIMER_FREQUENCY 10000

void timer_handler() {
	tick_count++;
	outp(0x20, 0x20);
}

_go32_dpmi_seginfo old_irq0, new_irq0;

void initialize_timer() {
	unsigned long divisor = 1193180 / TIMER_FREQUENCY;


	outp(0x43, 0x36);
	outp(0x40, divisor & 0xFF);
	outp(0x40, (divisor >> 8) & 0xFF);


	_go32_dpmi_get_protected_mode_interrupt_vector(0x8, &old_irq0);
	new_irq0.pm_offset = (int) timer_handler;
	new_irq0.pm_selector = _go32_my_cs();
	_go32_dpmi_allocate_iret_wrapper(&new_irq0);
	_go32_dpmi_set_protected_mode_interrupt_vector(0x8, &new_irq0);
}

void restore_timer() {

	_go32_dpmi_set_protected_mode_interrupt_vector(0x8, &old_irq0);
	_go32_dpmi_free_iret_wrapper(&new_irq0);
}
