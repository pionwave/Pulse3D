
// Written by Lynton "Pionwave" Schneider, 2024

#ifndef __PULSE_GRAPHICS_H__
#define __PULSE_GRAPHICS_H__

#include "bmp.h"
#include "Texture.h"

typedef struct {
	int x, y;
	float z, w, u, v;
	unsigned char color;
	float alpha;
} ScreenVertex;


void init_graphics();
void shutdown_graphics();

unsigned int graphics_screen_width();
unsigned int graphics_screen_height();

void clear_z_buffer();

void flip_doublebuffer();

void circle_slow(int x, int y, int radius, unsigned char color);

void draw_clipped_line(int x0, int y0, int x1, int y1, unsigned char color);
void draw_line(int x0, int y0, int x1, int y1, unsigned char color);
void draw_pixel(int x, int y, unsigned char color);
void draw_pixel_z_buffer(int x, int y, float z, unsigned char color);

void draw_filled_triangle(ScreenVertex v0, ScreenVertex v1, ScreenVertex v2);
void draw_bmp_triangle(ScreenVertex v0, ScreenVertex v1, ScreenVertex v2, const Bitmap *texture);
void draw_textured_triangle(ScreenVertex v0, ScreenVertex v1, ScreenVertex v2, const Texture *texture, float lod_level);
void clip_polygon(ScreenVertex *vertices, int vertex_count, ScreenVertex *output, int *output_count);

void sprite_draw(char *data, int x, int y, unsigned short width, unsigned short height);

void clear_screen();


#endif