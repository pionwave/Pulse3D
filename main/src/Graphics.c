
// Written by Lynton "Pionwave" Schneider, 2024
// thanks to http://www.brackeen.com/vga/ for the circle drawing

#include <dpmi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <math.h>
#include <sys/nearptr.h>

#include "Graphics.h"

unsigned char *double_buffer;
char *VGA = (char *) (0xA0000);

float *z_buffer = NULL;

#define WIDTH 320
#define HEIGHT 200

void initialize_z_buffer() {
	z_buffer = (float *) malloc(WIDTH * HEIGHT * sizeof(float));
	if (z_buffer == NULL) {
		printf("Not enough memory for Z-buffer.\n");
		exit(1);
	}
}

void free_z_buffer() {
	if (z_buffer !=
		NULL) {
		free(z_buffer);
		z_buffer =
				NULL;
	}
}


void clear_z_buffer() {
	for (int i = 0; i < 320 * 200; i++) {
		z_buffer[i] = 1000.0f;
	}
}

unsigned int graphics_screen_width() {
	return 320;
}

unsigned int graphics_screen_height() {
	return 200;
}


void set_mode_13h() {
	union REGS regs;
	regs.h.ah = 0x00;
	regs.h.al = 0x13;
	int86(0x10, &regs, &regs);
}

void set_mode_text() {
	union REGS regs;
	regs.h.ah = 0x00;
	regs.h.al = 0x03;
	int86(0x10, &regs, &regs);
}

void clear_screen() {

	memset(double_buffer, 0, WIDTH * HEIGHT);
}

void draw_pixel(int x, int y, unsigned char color) {


	double_buffer[(y << 8) + (y << 6) + x] = color;
}

void draw_pixel_z_buffer(int x, int y, float z, unsigned char color) {

	int index = (y << 8) + (y << 6) + x;
	if (z < z_buffer[index]) {
		double_buffer[index] = color;
		z_buffer[index] = z;
	}
}

void circle_slow(int x, int y, int radius, unsigned char color) {
	float n = 0, invradius = 1 / (float) radius;
	int dx = 0, dy = radius - 1;
	unsigned short dxoffset, dyoffset, offset = (y << 8) + (y << 6) + x;

	while (dx <= dy) {
		dxoffset = (dx << 8) + (dx << 6);
		dyoffset = (dy << 8) + (dy << 6);
		double_buffer[offset + dy - dxoffset] = color;
		double_buffer[offset + dx - dyoffset] = color;
		double_buffer[offset - dx - dyoffset] = color;
		double_buffer[offset - dy - dxoffset] = color;
		double_buffer[offset - dy + dxoffset] = color;
		double_buffer[offset - dx + dyoffset] = color;
		double_buffer[offset + dx + dyoffset] = color;
		double_buffer[offset + dy + dxoffset] = color;
		dx++;
		n += invradius;
		dy = radius * sin(acos(n));
	}
}

void draw_line(int x0, int y0, int x1, int y1, unsigned char color) {
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) {

			double_buffer[(y0 << 8) + (y0 << 6) + x0] = color;
		}
		if (x0 == x1 && y0 == y1) break;
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

int compute_out_code(int x, int y) {
	int code = 0;

	if (x < 0)
		code |= 1;
	else if (x >= WIDTH)
		code |= 2;
	if (y < 0)
		code |= 4;
	else if (y >= HEIGHT)
		code |= 8;

	return code;
}


void draw_clipped_line(int x0, int y0, int x1, int y1, unsigned char color) {
	int out_code0 = compute_out_code(x0, y0);
	int out_code1 = compute_out_code(x1, y1);
	int accept = 0;

	while (1) {
		if (!(out_code0 | out_code1)) {

			accept = 1;
			break;
		} else if (out_code0 & out_code1) {

			break;
		} else {

			int out_code_out = out_code0 ? out_code0 : out_code1;
			int x, y;

			if (out_code_out & 8) {
				x = x0 + (x1 - x0) * (HEIGHT - 1 - y0) / (y1 - y0);
				y = HEIGHT - 1;
			} else if (out_code_out & 4) {
				x = x0 + (x1 - x0) * (-y0) / (y1 - y0);
				y = 0;
			} else if (out_code_out & 2) {
				y = y0 + (y1 - y0) * (WIDTH - 1 - x0) / (x1 - x0);
				x = WIDTH - 1;
			} else if (out_code_out & 1) {
				y = y0 + (y1 - y0) * (-x0) / (x1 - x0);
				x = 0;
			}

			if (out_code_out == out_code0) {
				x0 = x;
				y0 = y;
				out_code0 = compute_out_code(x0, y0);
			} else {
				x1 = x;
				y1 = y;
				out_code1 = compute_out_code(x1, y1);
			}
		}
	}

	if (accept) {
		draw_line(x0, y0, x1, y1, color);
	}
}

void draw_filled_triangle(ScreenVertex v0, ScreenVertex v1, ScreenVertex v2) {

	if (v1.y < v0.y) {
		ScreenVertex temp = v0;
		v0 = v1;
		v1 = temp;
	}
	if (v2.y < v0.y) {
		ScreenVertex temp = v0;
		v0 = v2;
		v2 = temp;
	}
	if (v2.y < v1.y) {
		ScreenVertex temp = v1;
		v1 = v2;
		v2 = temp;
	}

	int total_height = v2.y - v0.y;


	if (total_height == 0) {
		return;
	}

	for (int i = 0; i <= total_height; i++) {
		int second_half = i > v1.y - v0.y || v1.y == v0.y;
		int segment_height = second_half ? v2.y - v1.y : v1.y - v0.y;
		float alpha = (float) i / total_height;
		float beta = (float) (i - (second_half ? v1.y - v0.y : 0)) / segment_height;

		int scanline_y = i + v0.y;

		ScreenVertex A = {
				v0.x + (v2.x - v0.x) * alpha,
				scanline_y,
				v0.z + (v2.z - v0.z) * alpha,
				v0.w + (v2.w - v0.w) * alpha,
				v0.u + (v2.u - v0.u) * alpha,
				v0.v + (v2.v - v0.v) * alpha,
				(unsigned char) (v0.color + (v2.color - v0.color) * alpha),
				v0.alpha + (v2.alpha - v0.alpha) * alpha};

		ScreenVertex B = second_half ? (ScreenVertex){
											   v1.x + (v2.x - v1.x) * beta,
											   scanline_y,
											   v1.z + (v2.z - v1.z) * beta,
											   v1.w + (v2.w - v1.w) * beta,
											   v1.u + (v2.u - v1.u) * beta,
											   v1.v + (v2.v - v1.v) * beta,
											   (unsigned char) (v1.color + (v2.color - v1.color) * beta),
											   v1.alpha + (v2.alpha - v1.alpha) * beta}
									 : (ScreenVertex){v0.x + (v1.x - v0.x) * beta, scanline_y, v0.z + (v1.z - v0.z) * beta, v0.w + (v1.w - v0.w) * beta, v0.u + (v1.u - v0.u) * beta, v0.v + (v1.v - v0.v) * beta, (unsigned char) (v0.color + (v1.color - v0.color) * beta), v0.alpha + (v1.alpha - v0.alpha) * beta};

		if (A.x > B.x) {
			ScreenVertex temp = A;
			A = B;
			B = temp;
		}

		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1.0f : (float) (j - A.x) / (B.x - A.x);
			ScreenVertex P = {
					j,
					scanline_y,
					A.z + (B.z - A.z) * phi,
					A.w + (B.w - A.w) * phi,
					A.u + (B.u - A.u) * phi,
					A.v + (B.v - A.v) * phi,
					(unsigned char) (A.color + (B.color - A.color) * phi),
					A.alpha + (B.alpha - A.alpha) * phi};
			draw_pixel(P.x, P.y, P.color);
		}
	}
}

unsigned char get_texture_color(const Bitmap *texture, float u, float v) {

	u = u - floor(u);
	v = v - floor(v);

	int tex_x = (int) (u * (texture->width - 1));
	int tex_y = (int) (v * (texture->height - 1));
	return texture->data[tex_y * texture->width + tex_x];
}

void draw_textured_triangle(ScreenVertex v0, ScreenVertex v1, ScreenVertex v2, const Bitmap *texture) {

	if (v1.y < v0.y) {
		ScreenVertex temp = v0;
		v0 = v1;
		v1 = temp;
	}
	if (v2.y < v0.y) {
		ScreenVertex temp = v0;
		v0 = v2;
		v2 = temp;
	}
	if (v2.y < v1.y) {
		ScreenVertex temp = v1;
		v1 = v2;
		v2 = temp;
	}

	int total_height = v2.y - v0.y;


	if (total_height == 0) {
		return;
	}

	float v0_inv_w = v0.w;
	float v1_inv_w = v1.w;
	float v2_inv_w = v2.w;


	float v0_u_over_w = v0.u;
	float v0_v_over_w = v0.v;

	float v1_u_over_w = v1.u;
	float v1_v_over_w = v1.v;

	float v2_u_over_w = v2.u;
	float v2_v_over_w = v2.v;


	for (int i = 0; i <= total_height; i++) {
		int second_half = i > v1.y - v0.y || v1.y == v0.y;
		int segment_height = second_half ? v2.y - v1.y : v1.y - v0.y;
		float alpha = (float) i / total_height;
		float beta = (float) (i - (second_half ? v1.y - v0.y : 0)) / segment_height;

		int scanline_y = i + v0.y;

		ScreenVertex A = {
				v0.x + (v2.x - v0.x) * alpha,
				scanline_y,
				0.0f,
				v0_inv_w + (v2_inv_w - v0_inv_w) * alpha,
				v0_u_over_w + (v2_u_over_w - v0_u_over_w) * alpha,
				v0_v_over_w + (v2_v_over_w - v0_v_over_w) * alpha};

		ScreenVertex B = second_half
								 ? (ScreenVertex){
										   v1.x + (v2.x - v1.x) * beta,
										   scanline_y,
										   0.0f,
										   v1_inv_w + (v2_inv_w - v1_inv_w) * beta,
										   v1_u_over_w + (v2_u_over_w - v1_u_over_w) * beta,
										   v1_v_over_w + (v2_v_over_w - v1_v_over_w) * beta}
								 : (ScreenVertex){v0.x + (v1.x - v0.x) * beta, scanline_y, 0.0f, v0_inv_w + (v1_inv_w - v0_inv_w) * beta, v0_u_over_w + (v1_u_over_w - v0_u_over_w) * beta, v0_v_over_w + (v1_v_over_w - v0_v_over_w) * beta};

		if (A.x > B.x) {
			ScreenVertex temp = A;
			A = B;
			B = temp;
		}

		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1.0f : (float) (j - A.x) / (B.x - A.x);

			float one_over_w = 1.0f / (A.w + phi * (B.w - A.w));
			float u_over_w = A.u + phi * (B.u - A.u);
			float v_over_w = A.v + phi * (B.v - A.v);


			float final_u = u_over_w * one_over_w;
			float final_v = v_over_w * one_over_w;

			unsigned char texture_color = get_texture_color(texture, final_u, final_v);


			draw_pixel(j, scanline_y, texture_color);
		}
	}
}

void copy_memory(void *dest, const void *src, size_t count) {
	asm volatile(
			"rep movsd"
			: "+D"(dest), "+S"(src), "+c"(count)
			:
			: "memory");
}

void flip_doublebuffer() {
	copy_memory(VGA, double_buffer, 16000);
}

void set_palette(int index, unsigned char r, unsigned char g, unsigned char b) {
	outp(0x3C8, index);
	outp(0x3C9, r);
	outp(0x3C9, g);
	outp(0x3C9, b);
}

void set_brown_palette() {

	int num_browns = 32;


	unsigned char start_r = 63, start_g = 50, start_b = 31;
	unsigned char end_r = 31, end_g = 15, end_b = 0;

	for (int i = 0; i < num_browns; ++i) {
		unsigned char r = start_r - (start_r - end_r) * i / (num_browns - 1);
		unsigned char g = start_g - (start_g - end_g) * i / (num_browns - 1);
		unsigned char b = start_b - (start_b - end_b) * i / (num_browns - 1);
		set_palette(16 + i, r, g, b);
	}


	set_palette(8, 32, 32, 32);


	set_palette(9, 0, 0, 63);
}

void init_graphics() {
	double_buffer = malloc(WIDTH * HEIGHT);
	if (double_buffer ==
		NULL) {
		printf("Error allocating double buffer memory!!");
		exit(1);
	}

	set_mode_13h();

	VGA += __djgpp_conventional_base;


	initialize_z_buffer();
}


void shutdown_graphics() {
	set_mode_text();

	if (double_buffer !=
		NULL) {
		free(double_buffer);
	}

	free_z_buffer();
}

ScreenVertex interpolate_screen_vertex(ScreenVertex v1, ScreenVertex v2, float t) {
	return (ScreenVertex){
			v1.x + t * (v2.x - v1.x),
			v1.y + t * (v2.y - v1.y),
			v1.z + t * (v2.z - v1.z),
			v1.w + t * (v2.w - v1.w),
			v1.u + t * (v2.u - v1.u),
			v1.v + t * (v2.v - v1.v),
			(unsigned char) (v1.color + t * (v2.color - v1.color)),
			v1.alpha + t * (v2.alpha - v1.alpha)};
}

int clip_polygon_to_plane(ScreenVertex *vertices, int vertex_count, ScreenVertex *output, int *output_count, int edge) {
	*output_count = 0;
	for (int i = 0; i < vertex_count; i++) {
		ScreenVertex current_vertex = vertices[i];
		ScreenVertex prev_vertex = vertices[(i + vertex_count - 1) % vertex_count];

		int current_out_code = compute_out_code(current_vertex.x, current_vertex.y);
		int prev_out_code = compute_out_code(prev_vertex.x, prev_vertex.y);

		int current_inside = !(current_out_code & edge);
		int prev_inside = !(prev_out_code & edge);

		if (current_inside != prev_inside) {
			float t;
			if (edge == 1)
				t = (0 - prev_vertex.x) / (float) (current_vertex.x - prev_vertex.x);
			else if (edge == 2)
				t = (320 - 1 - prev_vertex.x) / (float) (current_vertex.x - prev_vertex.x);
			else if (edge == 4)
				t = (0 - prev_vertex.y) / (float) (current_vertex.y - prev_vertex.y);
			else if (edge == 8)
				t = (200 - 1 - prev_vertex.y) / (float) (current_vertex.y - prev_vertex.y);

			ScreenVertex intersect_vertex = interpolate_screen_vertex(prev_vertex, current_vertex, t);
			output[(*output_count)++] = intersect_vertex;
		}

		if (current_inside) {
			output[(*output_count)++] = current_vertex;
		}
	}
	return *output_count;
}

void clip_polygon(ScreenVertex *vertices, int vertex_count, ScreenVertex *output, int *output_count) {
	ScreenVertex temp1[10], temp2[10];
	int count1 = vertex_count, count2;

	memcpy(temp1, vertices, vertex_count * sizeof(ScreenVertex));

	for (int edge = 1; edge <= 8; edge <<= 1) {
		clip_polygon_to_plane(temp1, count1, temp2, &count2, edge);
		memcpy(temp1, temp2, count2 * sizeof(ScreenVertex));
		count1 = count2;
	}

	*output_count = count1;
	memcpy(output, temp1, count1 * sizeof(ScreenVertex));
}

void sprite_draw(char *data, int x, int y, unsigned short width, unsigned short height) {
	int j;
	unsigned short screen_offset = (y << 8) + (y << 6) + x;
	unsigned short bitmap_offset = 0;

	for (j = 0; j < height; j++) {
		memcpy(&double_buffer[screen_offset], &data[bitmap_offset], width);

		bitmap_offset += width;
		screen_offset += 320;
	}
}
