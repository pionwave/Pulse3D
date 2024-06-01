
// Written by Lynton "Pionwave" Schneider, 2024

#ifndef __PULSE_MATH_H__
#define __PULSE_MATH_H__

#include <stdint.h>

static inline double fmin(double x, double y) {
	return (x < y) ? x : y;
}


static inline double fmax(double x, double y) {
	return (x > y) ? x : y;
}

static inline float fminf(float x, float y) {
	return (x < y) ? x : y;
}


static inline float fmaxf(float x, float y) {
	return (x > y) ? x : y;
}

static inline float fast_log2(float val) {
	union {
		float val;
		int32_t x;
	} u = {val};
	register float log_2 = (float) (((u.x >> 23) & 255) - 128);
	u.x &= ~(255 << 23);
	u.x += 127 << 23;
	log_2 += ((-0.3358287811f) * u.val + 2.0f) * u.val - 0.65871759316667f;
	return (log_2);
}


typedef struct
{
	float x, y, z;
} Vector3;

float fast_sqrt(float number);


#endif