
// Written by Lynton "Pionwave" Schneider, 2024

#include <dpmi.h>
#include <string.h>
#include <dos.h>
#include <sys/nearptr.h>

#include "Keyboard.h"

struct {
	char enabled;
} keyb_handler_info;

volatile char keyb[256];
volatile char last_keys[256];

unsigned char scancode2ascii[256] = {
		0, 0, 49, 50, 51, 52, 53, 54, 55, 56,
		57, 48, 45, 0, 0, 0, 113, 119, 101, 114,
		116, 121, 117, 105, 111, 112, 0, 0, 0, 0,
		97, 115, 100, 102, 103, 104, 106, 107, 108, 0,
		0, 0, 0, 0, 122, 120, 99, 118, 98, 110,
		109, 44, 46, 47, 0, 0, 0, 32, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0};

_go32_dpmi_seginfo old_keyb_handler_seginfo, new_keyb_handler_seginfo;

void keyb_handler() {
	unsigned char key;
	static char extended;
	int c1;

	extended = 0;
	key = inportb(0x60);

	if (key == 0xe0)
		extended = 1;
	else {
		if (extended == 0) {
			if ((key & 0x80) == 0) {
				keyb[key & 0x7f] = 1;
				for (c1 = 48; c1 > 0; c1--)
					last_keys[c1] = last_keys[c1 - 1];
				last_keys[0] = scancode2ascii[key & 0x7f];
			} else
				keyb[key & 0x7f] = 0;
		} else {
			if ((key & 0x80) == 0) {
				keyb[(key & 0x7f) + 0x80] = 1;
				for (c1 = 48; c1 > 0; c1--)
					last_keys[c1] = last_keys[c1 - 1];
				last_keys[0] = scancode2ascii[(key & 0x7f) + 0x80];
			} else
				keyb[(key & 0x7f) + 0x80] = 0;
		}
		if (extended == 1)
			extended = 0;
	}

	outportb(0x20, 0x20);
}

void keyb_handler_end() {}


char hook_keyb_handler(void) {

	if (keyb_handler_info.enabled == 0) {
		_go32_dpmi_lock_data((char *) &keyb, sizeof(keyb));
		_go32_dpmi_lock_code(keyb_handler, sizeof(keyb_handler));
		_go32_dpmi_get_protected_mode_interrupt_vector(9, &old_keyb_handler_seginfo);
		new_keyb_handler_seginfo.pm_offset = (int) keyb_handler;
		if (_go32_dpmi_allocate_iret_wrapper(&new_keyb_handler_seginfo) != 0)
			return 1;
		if (_go32_dpmi_set_protected_mode_interrupt_vector(9, &new_keyb_handler_seginfo) != 0) {
			_go32_dpmi_free_iret_wrapper(&new_keyb_handler_seginfo);
			return 1;
		}
		keyb_handler_info.enabled = 1;
		memset((char *) last_keys, 0, sizeof(last_keys));
	}

	return 0;
}


void remove_keyb_handler(void) {

	if (keyb_handler_info.enabled == 1) {
		_go32_dpmi_set_protected_mode_interrupt_vector(9, &old_keyb_handler_seginfo);
		_go32_dpmi_free_iret_wrapper(&new_keyb_handler_seginfo);
		keyb_handler_info.enabled = 0;
	}
}

void init_keyboard() {
	keyb_handler_info.enabled = 0;
	hook_keyb_handler();
}


void shutdown_keyboard() {

	remove_keyb_handler();
}
