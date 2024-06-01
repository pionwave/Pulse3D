
#ifndef __PULSE_TEXTURE_H__
#define __PULSE_TEXTURE_H__

#include "Bmp.h"

typedef struct
{
	unsigned int width;
	unsigned int height;
	unsigned char *data;
} Image;

typedef struct
{
	int num_lods;
	Image *mips;
} Texture;

Texture *create_texture_from_bmp(Bitmap *bmp);
void free_texture(Texture *tex);

#endif