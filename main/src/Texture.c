
#include "Texture.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Math.h"
#include <stdio.h>

#define LANCZOS_RADIUS 3

// Lanczos kernel function
double lanczos_kernel(double x) {
	if (x == 0.0) return 1.0;
	if (x < -LANCZOS_RADIUS || x > LANCZOS_RADIUS) return 0.0;
	x *= PI;
	return LANCZOS_RADIUS * sin(x) * sin(x / LANCZOS_RADIUS) / (x * x);
}

// Downsampling function
void downsample_lanczos(const unsigned char *input_image, unsigned char *output_image, int srcWidth, int srcHeight, int dstWidth, int dstHeight) {
	double xRatio = (double) srcWidth / dstWidth;
	double yRatio = (double) srcHeight / dstHeight;

	for (int dstY = 0; dstY < dstHeight; ++dstY) {
		for (int dstX = 0; dstX < dstWidth; ++dstX) {
			double srcX = dstX * xRatio;
			double srcY = dstY * yRatio;

			int xMin = (int) floor(srcX) - LANCZOS_RADIUS + 1;
			int xMax = (int) floor(srcX) + LANCZOS_RADIUS;
			int yMin = (int) floor(srcY) - LANCZOS_RADIUS + 1;
			int yMax = (int) floor(srcY) + LANCZOS_RADIUS;

			double pixelValue = 0.0;
			double weightSum = 0.0;

			for (int srcYk = yMin; srcYk <= yMax; ++srcYk) {
				for (int srcXk = xMin; srcXk <= xMax; ++srcXk) {
					if (srcXk >= 0 && srcXk < srcWidth && srcYk >= 0 && srcYk < srcHeight) {
						double weight = lanczos_kernel(srcX - srcXk) * lanczos_kernel(srcY - srcYk);
						int index = srcYk * srcWidth + srcXk;
						pixelValue += weight * input_image[index];
						weightSum += weight;
					}
				}
			}

			output_image[dstY * dstWidth + dstX] = (unsigned char) (fmax(0.0, fmin(255.0, pixelValue / weightSum)));
		}
	}
}


unsigned int count_right_shifts(unsigned int number, unsigned int final) {
	int count = 0;
	while (number > final) {
		number >>= 1;// Right shift the number by 1 bit (equivalent to dividing by 2)
		count++;
	}
	return count;
}

Texture *create_texture_from_bmp(Bitmap *bmp) {
	Texture *texture = malloc(sizeof(Texture));
	// count how many times we have to down sample to get 4x4 texture
	texture->num_lods = count_right_shifts(bmp->width, 4);

	texture->mips = malloc(sizeof(Image) * texture->num_lods);

	texture->mips[0].width = bmp->width;
	texture->mips[0].height = bmp->height;
	texture->mips[0].data = malloc(bmp->width * bmp->height);

	memcpy(texture->mips[0].data, bmp->data, bmp->width * bmp->height);

	unsigned int width = texture->mips[0].width;
	unsigned int height = texture->mips[0].height;
	unsigned int src_width = width;
	unsigned int src_height = height;

	for (unsigned int i = 1; i < texture->num_lods; i++) {
		width >>= 1;
		height >>= 1;

		Image *img = &texture->mips[i];

		img->width = width;
		img->height = height;

		img->data = malloc(img->width * img->height);
		downsample_lanczos(texture->mips[0].data, img->data, src_width, src_height, width, height);
	}

	return texture;
}

void free_texture(Texture *tex) {

	if (tex == NULL) {
		return;
	}

	for (unsigned int i = 0; i < tex->num_lods; i++) {
		if (tex->mips[i].data != NULL) {
			free(tex->mips[i].data);
			tex->mips[i].data = NULL;
		}
	}
	free(tex->mips);

	free(tex);
	tex = NULL;
}
