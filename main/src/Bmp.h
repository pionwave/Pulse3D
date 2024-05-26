
// Written by Lynton "Pionwave" Schneider, 2024

#ifndef __PULSE_BMP_H__
#define __PULSE_BMP_H__

typedef struct
{
	unsigned short width;
	unsigned short height;
	char palette[256 * 3];
	char *data;
} Bitmap;

int bmp_load(const char *filename, Bitmap *bitmap);
void bmp_set_palette(char *palette);
void bmp_draw(Bitmap *bmp, char *buffer, int x, int y, unsigned short screen_width);
void bmp_free_data(Bitmap *bitmap);

#endif