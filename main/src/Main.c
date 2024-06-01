
// Written by Lynton "Pionwave" Schneider, 2024


#include <dpmi.h>
#include <string.h>
#include <dos.h>
#include <sys/nearptr.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "Math.h"
#include "Graphics.h"
#include "Primitive.h"
#include "BSP.h"
#include "Keyboard.h"
#include "Timer.h"
#include "BMP.h"
#include "Texture.h"

unsigned walls[20][20] = {
		{8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
		{7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
		{7, 0, 8, 1, 1, 2, 0, 8, 1, 1, 2, 0, 8, 1, 1, 2, 0, 0, 0, 3},
		{7, 0, 7, 0, 0, 3, 0, 7, 0, 0, 3, 0, 7, 0, 0, 3, 0, 0, 0, 3},
		{7, 0, 7, 0, 0, 4, 0, 7, 0, 0, 4, 0, 7, 0, 0, 4, 0, 0, 0, 3},
		{7, 0, 7, 0, 0, 0, 0, 7, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 8, 1, 1, 6, 0, 8, 1, 1, 6, 0, 8, 1, 1, 1, 1, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{7, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 7, 0, 7, 0, 0, 0, 0, 3},
		{6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4}};


Vertex camera = {0, 8, 0};
float camera_yaw = 0.0f;
Matrix4 projection_matrix;
float TILE_SIZE = 16.0f;
float WALL_HEIGHT = 20.0f;
float playerRadius = 2.0f;
const unsigned ROWS = 20;
const unsigned COLS = 20;


Triangle ground_triangles[2];
Triangle sky_triangles[2];
Triangle wall_triangles[2048];
int num_wall_triangles;

Texture *texture_wall;
Texture *texture_concrete;

unsigned long last_tick = 0;
float frame_time = 0.0f;


int check_neighbor_collision(int col, int row, float rel_x, float rel_z);
int check_cell_collision(int col, int row, float rel_x, float rel_z);

int check_collision(float new_x, float new_z) {
	int col = (new_x + (COLS / 2) * TILE_SIZE) / TILE_SIZE;
	int row = (new_z + (ROWS / 2) * TILE_SIZE) / TILE_SIZE;

	if (col < 0 || col >= COLS || row < 0 || row >= ROWS) {
		return 1;
	}

	float rel_x = fmod(new_x + (COLS / 2) * TILE_SIZE, TILE_SIZE);
	float rel_z = fmod(new_z + (ROWS / 2) * TILE_SIZE, TILE_SIZE);


	return check_cell_collision(col, row, rel_x, rel_z) ||
		   check_neighbor_collision(col, row, rel_x, rel_z);
}

int check_cell_collision(int col, int row, float rel_x, float rel_z) {
	if (col < 0 || col >= COLS || row < 0 || row >= ROWS) {
		return 1;
	}

	int wall_type = walls[row][col];
	switch (wall_type) {
		case 1:
			if (rel_z < playerRadius) return 1;
			break;
		case 2:
			if (rel_z < playerRadius || rel_x > (TILE_SIZE - playerRadius)) return 1;
			break;
		case 3:
			if (rel_x > (TILE_SIZE - playerRadius)) return 1;
			break;
		case 4:
			if (rel_x > (TILE_SIZE - playerRadius) || rel_z > (TILE_SIZE - playerRadius)) return 1;
			break;
		case 5:
			if (rel_z > (TILE_SIZE - playerRadius)) return 1;
			break;
		case 6:
			if (rel_z > (TILE_SIZE - playerRadius) || rel_x < playerRadius) return 1;
			break;
		case 7:
			if (rel_x < playerRadius) return 1;
			break;
		case 8:
			if (rel_z < playerRadius || rel_x < playerRadius) return 1;
			break;
		default:
			break;
	}

	return 0;
}

int check_neighbor_collision(int col, int row, float rel_x, float rel_z) {

	int collision = 0;
	if (rel_x < playerRadius) {
		collision |= check_cell_collision(col - 1, row, rel_x + TILE_SIZE, rel_z);
	}
	if (rel_x > (TILE_SIZE - playerRadius)) {
		collision |= check_cell_collision(col + 1, row, rel_x - TILE_SIZE, rel_z);
	}
	if (rel_z < playerRadius) {
		collision |= check_cell_collision(col, row - 1, rel_x, rel_z + TILE_SIZE);
	}
	if (rel_z > (TILE_SIZE - playerRadius)) {
		collision |= check_cell_collision(col, row + 1, rel_x, rel_z - TILE_SIZE);
	}


	if (rel_x < playerRadius && rel_z < playerRadius) {
		collision |= check_cell_collision(col - 1, row - 1, rel_x + TILE_SIZE, rel_z + TILE_SIZE);
	}
	if (rel_x > (TILE_SIZE - playerRadius) && rel_z < playerRadius) {
		collision |= check_cell_collision(col + 1, row - 1, rel_x - TILE_SIZE, rel_z + TILE_SIZE);
	}
	if (rel_x < playerRadius && rel_z > (TILE_SIZE - playerRadius)) {
		collision |= check_cell_collision(col - 1, row + 1, rel_x + TILE_SIZE, rel_z - TILE_SIZE);
	}
	if (rel_x > (TILE_SIZE - playerRadius) && rel_z > (TILE_SIZE - playerRadius)) {
		collision |= check_cell_collision(col + 1, row + 1, rel_x - TILE_SIZE, rel_z - TILE_SIZE);
	}

	return collision;
}

void handle_input(int *quit) {
	float rate = (float)
			M_PI;
	float moveRate = 80.0f;
	float new_x, new_z;

	if (keyb[75]) {
		camera_yaw += rate * frame_time;
	}

	if (keyb[77]) {
		camera_yaw -= rate * frame_time;
	}

	if (keyb[72]) {
		new_x = camera.x + sin(camera_yaw) * moveRate * frame_time;
		new_z = camera.z + cos(camera_yaw) * moveRate * frame_time;
		if (!check_collision(new_x, new_z)) {
			camera.x = new_x;
			camera.z = new_z;
		} else {

			float slide_x = camera.x;
			float slide_z = camera.z;

			if (!check_collision(new_x, camera.z)) {
				camera.x = new_x;
			} else if (!check_collision(camera.x, new_z)) {
				camera.z = new_z;
			}
		}
	}

	if (keyb[80]) {
		new_x = camera.x - sin(camera_yaw) * moveRate * frame_time;
		new_z = camera.z - cos(camera_yaw) * moveRate * frame_time;
		if (!check_collision(new_x, new_z)) {
			camera.x = new_x;
			camera.z = new_z;
		} else {

			float slide_x = camera.x;
			float slide_z = camera.z;

			if (!check_collision(new_x, camera.z)) {
				camera.x = new_x;
			} else if (!check_collision(camera.x, new_z)) {
				camera.z = new_z;
			}
		}
	}

	if (keyb[1]) {
		*quit = 1;
	}
}

void create_triangles() {
	num_wall_triangles = 0;

	float offsetx = (COLS * TILE_SIZE) * 0.5f;
	float offsetz = (ROWS * TILE_SIZE) * 0.5f;


	float extra = 1.25f;

	Vertex floor_v0 = {-offsetx * extra, 0, -offsetz * extra, 0.0f, 0.0f, 8, 1.0f};
	Vertex floor_v1 = {offsetx * extra, 0, -offsetz * extra, COLS / 2.0f, 0.0f, 8, 1.0f};
	Vertex floor_v2 = {offsetx * extra, 0, offsetz * extra, COLS / 2.0f, ROWS / 2.0f, 8, 1.0f};
	Vertex floor_v3 = {-offsetx * extra, 0, offsetz * extra, 0.0f, ROWS / 2.0f, 8, 1.0f};

	ground_triangles[0] = (Triangle){floor_v0, floor_v1, floor_v2};
	ground_triangles[1] = (Triangle){floor_v0, floor_v2, floor_v3};


	Vertex sky_v0 = {-offsetx * extra, WALL_HEIGHT, -offsetz * extra, 0.0f, 0.0f, 9, 1.0f};
	Vertex sky_v1 = {offsetx * extra, WALL_HEIGHT, -offsetz * extra, 1.0f, 0.0f, 9, 1.0f};
	Vertex sky_v2 = {offsetx * extra, WALL_HEIGHT, offsetz * extra, 1.0f, 1.0f, 9, 1.0f};
	Vertex sky_v3 = {-offsetx * extra, WALL_HEIGHT, offsetz * extra, 0.0f, 1.0f, 9, 1.0f};

	sky_triangles[0] = (Triangle){sky_v0, sky_v1, sky_v2};
	sky_triangles[1] = (Triangle){sky_v0, sky_v2, sky_v3};

	for (int row = 0; row < ROWS; row++) {
		for (int col = 0; col < COLS; col++) {
			int wall_type = walls[row][col];
			float x = (col * TILE_SIZE) - offsetx;
			float z = (row * TILE_SIZE) - offsetz;


			Vertex v0 = {x, 0, z, 0.0f, 0.0f, 16, 1.0f};
			Vertex v1 = {x + TILE_SIZE, 0, z, 1.0f, 0.0f, 16, 1.0f};
			Vertex v2 = {x + TILE_SIZE, 0, z + TILE_SIZE, 1.0f, 1.0f, 16, 1.0f};
			Vertex v3 = {x, 0, z + TILE_SIZE, 0.0f, 1.0f, 16, 1.0f};

			Vertex v4 = {x, WALL_HEIGHT, z, 0.0f, 0.0f, 47, 1.0f};
			Vertex v5 = {x + TILE_SIZE, WALL_HEIGHT, z, 1.0f, 0.0f, 47, 1.0f};
			Vertex v6 = {x + TILE_SIZE, WALL_HEIGHT, z + TILE_SIZE, 1.0f, 1.0f, 47, 1.0f};
			Vertex v7 = {x, WALL_HEIGHT, z + TILE_SIZE, 0.0f, 1.0f, 47, 1.0f};

			switch (wall_type) {
				case 1:
					wall_triangles[num_wall_triangles++] = (Triangle){v0, v1, v4};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;

					wall_triangles[num_wall_triangles++] = (Triangle){v1, v5, v4};

					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					break;

				case 2:

					wall_triangles[num_wall_triangles++] = (Triangle){v0, v1, v4};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v1, v5, v4};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;

					wall_triangles[num_wall_triangles++] = (Triangle){v1, v2, v5};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v2, v6, v5};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					break;

				case 3:
					wall_triangles[num_wall_triangles++] = (Triangle){v1, v2, v5};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v2, v6, v5};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					break;

				case 4:

					wall_triangles[num_wall_triangles++] = (Triangle){v1, v2, v5};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v2, v6, v5};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;

					wall_triangles[num_wall_triangles++] = (Triangle){v2, v3, v6};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v3, v7, v6};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					break;

				case 5:
					wall_triangles[num_wall_triangles++] = (Triangle){v2, v3, v6};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v3, v7, v6};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					break;

				case 6:

					wall_triangles[num_wall_triangles++] = (Triangle){v2, v3, v6};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v3, v7, v6};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;

					wall_triangles[num_wall_triangles++] = (Triangle){v3, v0, v7};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v0, v4, v7};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					break;

				case 7:
					wall_triangles[num_wall_triangles++] = (Triangle){v3, v0, v7};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v0, v4, v7};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					break;

				case 8:

					wall_triangles[num_wall_triangles++] = (Triangle){v0, v1, v4};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v1, v5, v4};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;

					wall_triangles[num_wall_triangles++] = (Triangle){v3, v0, v7};
					wall_triangles[num_wall_triangles - 1].v0.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					wall_triangles[num_wall_triangles++] = (Triangle){v0, v4, v7};
					wall_triangles[num_wall_triangles - 1].v0.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v0.v = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.u = 1.0f;
					wall_triangles[num_wall_triangles - 1].v1.v = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.u = 0.0f;
					wall_triangles[num_wall_triangles - 1].v2.v = 0.0f;
					break;

				default:
					break;
			}
		}
	}
}


ScreenVertex convert_to_screen_vertex(Vertex v) {
	int screen_x, screen_y;
	float screen_z, screen_w;
	project(v, projection_matrix, &screen_x, &screen_y, &screen_z, &screen_w);

	// extremely important to store w as 1/w and u & v as u/w & v/w. This allows them to be
	// linearly interpolated in screen space.
	return (ScreenVertex){screen_x, screen_y, screen_z, 1.0f / screen_w, v.u / screen_w, v.v / screen_w, v.color, v.alpha};
}


int clip_triangle_to_near_plane(Triangle input, Triangle *output1, Triangle *output2) {
	const float near_z = 1.0f;
	Vertex v[3] = {input.v0, input.v1, input.v2};
	Vertex in[3], out[3];
	int in_count = 0, out_count = 0;


	for (int i = 0; i < 3; i++) {
		if (v[i].z >= near_z) {
			in[in_count++] = v[i];
		} else {
			out[out_count++] = v[i];
		}
	}


	if (in_count == 0) {
		return 0;
	}


	if (in_count == 3) {
		*output1 = input;
		return 1;
	}


	if (in_count == 1 && out_count == 2) {
		Vertex new_v1 = interpolate_vertex(in[0], out[0], (near_z - in[0].z) / (out[0].z - in[0].z));
		Vertex new_v2 = interpolate_vertex(in[0], out[1], (near_z - in[0].z) / (out[1].z - in[0].z));

		*output1 = (Triangle){in[0], new_v1, new_v2};
		return 1;
	}


	if (in_count == 2 && out_count == 1) {
		Vertex new_v1 = interpolate_vertex(in[0], out[0], (near_z - in[0].z) / (out[0].z - in[0].z));
		Vertex new_v2 = interpolate_vertex(in[1], out[0], (near_z - in[1].z) / (out[0].z - in[1].z));

		*output1 = (Triangle){in[0], in[1], new_v1};
		*output2 = (Triangle){in[1], new_v1, new_v2};
		return 2;
	}

	return 0;
}


void draw_clipped_filled_triangle(Triangle triangle) {
	Triangle clipped_triangles[2];
	int clipped_count = clip_triangle_to_near_plane(triangle, &clipped_triangles[0], &clipped_triangles[1]);

	for (int i = 0; i < clipped_count; i++) {
		ScreenVertex sv0 = convert_to_screen_vertex(clipped_triangles[i].v0);
		ScreenVertex sv1 = convert_to_screen_vertex(clipped_triangles[i].v1);
		ScreenVertex sv2 = convert_to_screen_vertex(clipped_triangles[i].v2);

		ScreenVertex vertices[3] = {sv0, sv1, sv2};
		ScreenVertex clipped_vertices[10];
		int clipped_vertex_count;

		clip_polygon(vertices, 3, clipped_vertices, &clipped_vertex_count);


		for (int j = 2; j < clipped_vertex_count; j++) {
			draw_filled_triangle(clipped_vertices[0], clipped_vertices[j - 1], clipped_vertices[j]);
		}
	}
}

void draw_clipped_textured_triangle(Triangle triangle, Texture *texture) {
	Triangle clipped_triangles[2];
	int clipped_count = clip_triangle_to_near_plane(triangle, &clipped_triangles[0], &clipped_triangles[1]);

	for (int i = 0; i < clipped_count; i++) {
		ScreenVertex sv0 = convert_to_screen_vertex(clipped_triangles[i].v0);
		ScreenVertex sv1 = convert_to_screen_vertex(clipped_triangles[i].v1);
		ScreenVertex sv2 = convert_to_screen_vertex(clipped_triangles[i].v2);

		ScreenVertex vertices[3] = {sv0, sv1, sv2};
		ScreenVertex clipped_vertices[10];
		int clipped_vertex_count;

		clip_polygon(vertices, 3, clipped_vertices, &clipped_vertex_count);


		for (int j = 2; j < clipped_vertex_count; j++) {
			draw_textured_triangle(clipped_vertices[0], clipped_vertices[j - 1], clipped_vertices[j], texture, 0);
		}
	}
}

int depth_test(const void *a, const void *b) {
	Triangle *tri_a = (Triangle *) a;
	Triangle *tri_b = (Triangle *) b;


	Vertex transformed_v0_a = transform_vertex(tri_a->v0, camera, camera_yaw);
	Vertex transformed_v1_a = transform_vertex(tri_a->v1, camera, camera_yaw);
	Vertex transformed_v2_a = transform_vertex(tri_a->v2, camera, camera_yaw);


	float z_a = (transformed_v0_a.z + transformed_v1_a.z + transformed_v2_a.z) / 3.0f;


	Vertex transformed_v0_b = transform_vertex(tri_b->v0, camera, camera_yaw);
	Vertex transformed_v1_b = transform_vertex(tri_b->v1, camera, camera_yaw);
	Vertex transformed_v2_b = transform_vertex(tri_b->v2, camera, camera_yaw);


	float z_b = (transformed_v0_b.z + transformed_v1_b.z + transformed_v2_b.z) / 3.0f;

	return (z_b - z_a) > 0 ? 1 : -1;
}

int is_sphere_touch_cone(Sphere sphere, ConeFrustum cone) {
	Vertex to_center = subtract(sphere.center, cone.apex);
	float to_len_sq = dot_product(to_center, to_center);
	float v1_len = dot_product(to_center, cone.direction);
	float distance_closest_point = cone.cos_fov * fast_sqrt(to_len_sq - (v1_len * v1_len)) - v1_len * cone.sin_fov;

	int angle_cull = distance_closest_point > sphere.radius;
	int front_cull = v1_len > sphere.radius + cone.height;
	int back_cull = v1_len < -sphere.radius;

	return !(angle_cull || front_cull || back_cull);
}

int is_aabb_in_cone(AABB *box, ConeFrustum *frustum) {
	Sphere sphere = calculate_bounding_sphere_from_aabb(box->min, box->max);

	return is_sphere_touch_cone(sphere, *frustum);
}

int is_aabb_in_frustum(AABB *box, CameraFrustum *frustum) {
	Sphere sphere = calculate_bounding_sphere_from_aabb(box->min, box->max);

	return


			is_on_or_forward_plane(&frustum->leftPlane, &sphere) &&
			is_on_or_forward_plane(&frustum->rightPlane, &sphere);
}

void draw_cone_frustum(ConeFrustum *cone, Vertex right_vec, float scale, unsigned char color) {
	const int offsetx = 160;
	const int offsety = 90;


	int apexX = (int) (scale * cone->apex.x) + offsetx;
	int apexY = (int) (scale * cone->apex.z) + offsety;


	float dirX = cone->direction.x;
	float dirY = cone->direction.z;

	float rdX = right_vec.x;
	float rdY = right_vec.z;


	int farX = (int) ((scale * cone->apex.x) + dirX * (cone->height * scale)) + offsetx;
	int farY = (int) ((scale * cone->apex.z) + dirY * (cone->height * scale)) + offsety;


	float radius = (cone->radius * scale);

	int leftX = farX + (-radius * rdX);
	int leftY = farY + (-radius * rdY);

	int rightX = farX + (radius * rdX);
	int rightY = farY + (radius * rdY);

	draw_clipped_line(apexX, apexY, farX, farY, color);


	draw_clipped_line(apexX, apexY, leftX, leftY, color);
	draw_clipped_line(apexX, apexY, rightX, rightY, color);
}

void draw_aabb(AABB *aabb, float scale, unsigned char color) {
	const int offsetx = 160;
	const int offsety = 90;

	Vertex min = aabb->min;
	Vertex max = aabb->max;
	Vertex ctr;
	ctr.x = (aabb->min.x + aabb->max.x) * 0.5f;
	ctr.y = (aabb->min.y + aabb->max.y) * 0.5f;
	ctr.z = (aabb->min.z + aabb->max.z) * 0.5f;

	int minX = (int) (scale * min.x) + offsetx;
	int minZ = (int) (scale * min.z) + offsety;
	int maxX = (int) (scale * max.x) + offsetx;
	int maxZ = (int) (scale * max.z) + offsety;

	Sphere sphere = calculate_bounding_sphere_from_aabb(min, max);

	circle_slow((sphere.center.x * scale) + offsetx, (sphere.center.z * scale) + offsety, sphere.radius * scale, 14);


	draw_clipped_line(minX, minZ, maxX, minZ, color);
	draw_clipped_line(minX, maxZ, maxX, maxZ, color);
	draw_clipped_line(minX, minZ, minX, maxZ, color);
	draw_clipped_line(maxX, minZ, maxX, maxZ, color);

	draw_pixel((ctr.x * scale) + offsetx, (ctr.z * scale) + offsety, 14);
}

int is_vertex_in_aabb(Vertex v, AABB *aabb) {
	if (v.x >= aabb->min.x && v.x <= aabb->max.x &&
		v.y >= aabb->min.y && v.y <= aabb->max.y &&
		v.z >= aabb->min.z && v.z <= aabb->max.z) {
		return 1;
	} else {
		return 0;
	}
}

int is_vertex_in_aabb_xz(Vertex v, AABB *aabb) {
	if (v.x >= aabb->min.x && v.x <= aabb->max.x &&
		v.z >= aabb->min.z && v.z <= aabb->max.z) {
		return 1;
	} else {
		return 0;
	}
}

void render_aabb_bsp(BSPNode *node, ConeFrustum *frustum) {
	if (node == NULL) return;

	if (is_aabb_in_cone(&node->bounding_box, frustum)) {


		render_aabb_bsp(node->front, frustum);
		render_aabb_bsp(node->back, frustum);

	} else {

		draw_aabb(&node->bounding_box, 0.25f, 4);
	}
}


void render_bsp_tree(BSPNode *node, float px, float py, float pz, ConeFrustum *frustum) {
	if (node ==
		NULL) return;


	if (!is_aabb_in_cone(&node->bounding_box, frustum)) {


		return;
	}


	float nx, ny, nz;
	calculate_normal(node->triangle.v0, node->triangle.v1, node->triangle.v2, &nx, &ny, &nz);


	float distance = (px - node->triangle.v0.x) * nx + (py - node->triangle.v0.y) * ny + (pz - node->triangle.v0.z) * nz;

	if (distance > 0) {
		render_bsp_tree(node->back, px, py, pz, frustum);


		Triangle transformed_triangle = {
				transform_vertex(node->triangle.v0, camera, camera_yaw),
				transform_vertex(node->triangle.v1, camera, camera_yaw),
				transform_vertex(node->triangle.v2, camera, camera_yaw)};
		draw_clipped_textured_triangle(transformed_triangle, texture_wall);

		render_bsp_tree(node->front, px, py, pz, frustum);

	} else {
		render_bsp_tree(node->front, px, py, pz, frustum);


		Triangle transformed_triangle = {
				transform_vertex(node->triangle.v0, camera, camera_yaw),
				transform_vertex(node->triangle.v1, camera, camera_yaw),
				transform_vertex(node->triangle.v2, camera, camera_yaw)};
		draw_clipped_textured_triangle(transformed_triangle, texture_wall);

		render_bsp_tree(node->back, px, py, pz, frustum);
	}
}

int main() {
	create_triangles();

	BSPNode *bsp_tree = build_bsp_tree(wall_triangles, num_wall_triangles);

	Bitmap bmp_wall;
	Bitmap bmp_concrete;

	if (!bmp_load("data/textures/panel.BMP", &bmp_wall)) {
		printf("Failed to load bitmap: %s\n", "data/textures/panel.BMP");
		exit(1);
	}

	if (!bmp_load("data/textures/concrete.BMP", &bmp_concrete)) {
		printf("Failed to load bitmap: %s\n", "data/textures/concrete.BMP");
		exit(1);
	}

	if (__djgpp_nearptr_enable() == 0) {
		printf("Could get access to first 640K of memory.\n");
		exit(-1);
	}

	texture_wall = create_texture_from_bmp(&bmp_wall);
	texture_concrete = create_texture_from_bmp(&bmp_concrete);

	initialize_timer();
	init_graphics();
	init_keyboard();


	int quit = 0;

	create_projection_matrix(&projection_matrix, 320.0f, 200.0f, 60.0f, 1.0f, 1000.0f);

	last_tick = tick_count;

	ConeFrustum frustum;


	Vertex forward_vec;
	Vertex right_vec;
	Vertex up_vec;

	up_vec.x = 0.0f;
	up_vec.y = 1.0f;
	up_vec.z = 0.0f;

	float aspect = 320.0f / 200.0f;

	create_cone_frustum(&frustum, camera, forward_vec, 60.0f, 1000.0f, 5.0f);

	bmp_set_palette(bmp_wall.palette);

	while (quit == 0) {
		float fx = sin(camera_yaw);
		float fz = cos(camera_yaw);
		float fy = 0.0f;
		forward_vec.x = fx;
		forward_vec.y = fy;
		forward_vec.z = fz;
		right_vec = cross_product(forward_vec, up_vec);


		if (tick_count > last_tick) {
			unsigned short frames = tick_count - last_tick;

			frame_time = (frames * 0.1f) * 0.001f;

			handle_input(&quit);


			float fx = sin(camera_yaw);
			float fz = cos(camera_yaw);
			float fy = 0.0f;

			forward_vec.x = fx;
			forward_vec.y = fy;
			forward_vec.z = fz;


			create_cone_frustum(&frustum, camera, forward_vec, 60.0f, 1000.0f, 5.0f);

			last_tick = tick_count;
		}

		clear_z_buffer();


		for (int i = 0; i < 2; i++) {
			Triangle transformed_triangle = {
					transform_vertex(ground_triangles[i].v0, camera, camera_yaw),
					transform_vertex(ground_triangles[i].v1, camera, camera_yaw),
					transform_vertex(ground_triangles[i].v2, camera, camera_yaw)};
			draw_clipped_textured_triangle(transformed_triangle, texture_concrete);
		}


		for (int i = 0; i < 2; i++) {
			Triangle transformed_triangle = {
					transform_vertex(sky_triangles[i].v0, camera, camera_yaw),
					transform_vertex(sky_triangles[i].v1, camera, camera_yaw),
					transform_vertex(sky_triangles[i].v2, camera, camera_yaw)};
			draw_clipped_filled_triangle(transformed_triangle);
		}

		render_bsp_tree(bsp_tree, camera.x, camera.y, camera.z, &frustum);

		flip_doublebuffer();
	}

	free_bsp_tree(bsp_tree);
	bmp_free_data(&bmp_wall);
	bmp_free_data(&bmp_concrete);
	free_texture(texture_concrete);
	free_texture(texture_wall);

	restore_timer();
	shutdown_graphics();

	__djgpp_nearptr_disable();
	shutdown_keyboard();
	return 0;
}
