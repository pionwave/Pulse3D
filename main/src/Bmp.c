
// Written by Lynton "Pionwave" Schneider, 2024

#include <dpmi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

#include "bmp.h"

#pragma pack(push, 1)
typedef struct {
	unsigned short bfType;
	unsigned int bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
	unsigned int biSize;
	int biWidth;
	int biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int biCompression;
	unsigned int biSizeImage;
	int biXPelsPerMeter;
	int biYPelsPerMeter;
	unsigned int biClrUsed;
	unsigned int biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

typedef struct {
	unsigned char rgbBlue;
	unsigned char rgbGreen;
	unsigned char rgbRed;
	unsigned char rgbReserved;
} RGBQUAD;

int bmp_load(const char *filename, Bitmap *bitmap) {
	FILE *file = fopen(filename, "rb");
	if (!file) {


		return 0;
	}

	BITMAPFILEHEADER bfh;
	fread(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
	if (bfh.bfType != 0x4D42) {

		fclose(file);

		return 0;
	}

	BITMAPINFOHEADER bih;
	fread(&bih, sizeof(BITMAPINFOHEADER), 1, file);
	if (bih.biBitCount != 8) {


		fclose(file);
		return 0;
	}

	bitmap->width = bih.biWidth;
	bitmap->height = bih.biHeight;
	bitmap->data = (char *) malloc(bih.biSizeImage);
	if (!bitmap->data) {

		fclose(file);
		return 0;
	}


	RGBQUAD palette[256];
	fread(palette, sizeof(RGBQUAD), 256, file);
	for (int i = 0; i < 256; ++i) {
		bitmap->palette[i * 3] = palette[i].rgbRed;
		bitmap->palette[i * 3 + 1] = palette[i].rgbGreen;
		bitmap->palette[i * 3 + 2] = palette[i].rgbBlue;
	}


	fseek(file, bfh.bfOffBits,
		  SEEK_SET);


	for (int y = 0; y < bih.biHeight; ++y) {
		fread(&bitmap->data[(bih.biHeight - 1 - y) * bih.biWidth], 1, bih.biWidth, file);
	}

	fclose(file);
	return 1;
}

void bmp_set_palette(char *palette) {
	int i;

	outp(0x03c8, 0);

	for (i = 0; i < 256 * 3; i++)
		outp(0x03c9, palette[i] >> 2);
}

void bmp_draw(Bitmap *bmp, char *buffer, int x, int y, unsigned short screen_width) {
	int j;
	unsigned short screen_offset = (y << 8) + (y << 6) + x;
	unsigned short bitmap_offset = 0;

	for (j = 0; j < bmp->height; j++) {
		memcpy(&buffer[screen_offset], &bmp->data[bitmap_offset], bmp->width);

		bitmap_offset += bmp->width;
		screen_offset += screen_width;
	}
}

void bmp_free_data(Bitmap *bitmap) {
	if (bitmap != NULL) {
		if (bitmap->data != NULL) {
			free(bitmap->data);
			bitmap->data = NULL;
		}
	}
}
