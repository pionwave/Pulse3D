
// Written by Lynton "Pionwave" Schneider, 2024

#include <dpmi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

#include "Primitive.h"
#include "BSP.h"
#include "Math.h"

int classify_vertex(Vertex v, Vertex plane_point, float nx, float ny, float nz) {
	float d = (v.x - plane_point.x) * nx + (v.y - plane_point.y) * ny + (v.z - plane_point.z) * nz;
	if (d > 0.0001f) return 1;
	if (d < -0.0001f) return -1;
	return 0;
}


static inline float fminf(float x, float y) {
	return (x < y) ? x : y;
}


static inline float fmaxf(float x, float y) {
	return (x > y) ? x : y;
}

void calculate_aabb(Triangle *triangle, AABB *box) {
	box->min.x = fminf(triangle->v0.x, fminf(triangle->v1.x, triangle->v2.x));
	box->min.y = fminf(triangle->v0.y, fminf(triangle->v1.y, triangle->v2.y));
	box->min.z = fminf(triangle->v0.z, fminf(triangle->v1.z, triangle->v2.z));

	box->max.x = fmaxf(triangle->v0.x, fmaxf(triangle->v1.x, triangle->v2.x));
	box->max.y = fmaxf(triangle->v0.y, fmaxf(triangle->v1.y, triangle->v2.y));
	box->max.z = fmaxf(triangle->v0.z, fmaxf(triangle->v1.z, triangle->v2.z));
}

void aabb_grow(AABB *result, AABB *left, AABB *right) {

	result->min.x = fminf(left->min.x, right->min.x);
	result->min.y = fminf(left->min.y, right->min.y);
	result->min.z = fminf(left->min.z, right->min.z);

	result->max.x = fmaxf(left->max.x, right->max.x);
	result->max.y = fmaxf(left->max.y, right->max.y);
	result->max.z = fmaxf(left->max.z, right->max.z);
}


int split_triangle(Triangle input, Vertex plane_point, float nx, float ny, float nz, Triangle *front1, Triangle *front2, Triangle *back1, Triangle *back2) {
	Vertex vertices[3] = {input.v0, input.v1, input.v2};
	Vertex front_vertices[4], back_vertices[4];
	int front_count = 0, back_count = 0, coplanar = 0;
	Vertex coplanar_vertices[4];

	for (int i = 0; i < 3; i++) {
		float distance = (vertices[i].x - plane_point.x) * nx + (vertices[i].y - plane_point.y) * ny + (vertices[i].z - plane_point.z) * nz;
		if (distance > 0.0001f) {
			front_vertices[front_count++] = vertices[i];
		} else if (distance < -0.0001f) {
			back_vertices[back_count++] = vertices[i];
		} else {
			coplanar_vertices[coplanar++];
		}
	}


	if (front_count == 0 || back_count == 0) {
		return 0;
	}

	if (coplanar == 2) {
		return 0;
	}

	if (front_count == 2 && coplanar == 1) {
		return 0;
	}

	if (back_count == 2 && coplanar == 1) {
		return 0;
	}


	if (front_count == 1 && back_count == 2) {

		front_vertices[1] = interpolate_vertex(front_vertices[0], back_vertices[0],
											   (-(back_vertices[0].x - plane_point.x) * nx - (back_vertices[0].y - plane_point.y) * ny - (back_vertices[0].z - plane_point.z) * nz) /
													   ((front_vertices[0].x - back_vertices[0].x) * nx + (front_vertices[0].y - back_vertices[0].y) * ny + (front_vertices[0].z - back_vertices[0].z) * nz));

		front_vertices[2] = interpolate_vertex(front_vertices[0], back_vertices[1],
											   (-(back_vertices[1].x - plane_point.x) * nx - (back_vertices[1].y - plane_point.y) * ny - (back_vertices[1].z - plane_point.z) * nz) /
													   ((front_vertices[0].x - back_vertices[1].x) * nx + (front_vertices[0].y - back_vertices[1].y) * ny + (front_vertices[0].z - back_vertices[1].z) * nz));

		back_vertices[2] = front_vertices[1];
		back_vertices[3] = front_vertices[2];

		*front1 = (Triangle){front_vertices[0], front_vertices[1], front_vertices[2]};
		*back1 = (Triangle){back_vertices[0], back_vertices[1], back_vertices[2]};
		*back2 = (Triangle){back_vertices[0], back_vertices[2], back_vertices[3]};

		return 2;
	} else if (front_count == 2 && back_count == 1) {

		back_vertices[2] = interpolate_vertex(back_vertices[0], front_vertices[0],
											  (-(front_vertices[0].x - plane_point.x) * nx - (front_vertices[0].y - plane_point.y) * ny - (front_vertices[0].z - plane_point.z) * nz) /
													  ((back_vertices[0].x - front_vertices[0].x) * nx + (back_vertices[0].y - front_vertices[0].y) * ny + (back_vertices[0].z - front_vertices[0].z) * nz));

		back_vertices[3] = interpolate_vertex(back_vertices[0], front_vertices[1],
											  (-(front_vertices[1].x - plane_point.x) * nx - (front_vertices[1].y - plane_point.y) * ny - (front_vertices[1].z - plane_point.z) * nz) /
													  ((back_vertices[0].x - front_vertices[1].x) * nx + (back_vertices[0].y - front_vertices[1].y) * ny + (back_vertices[0].z - front_vertices[1].z) * nz));

		front_vertices[2] = back_vertices[2];
		front_vertices[3] = back_vertices[3];

		*front1 = (Triangle){front_vertices[0], front_vertices[1], front_vertices[2]};
		*front2 = (Triangle){front_vertices[0], front_vertices[2], front_vertices[3]};
		*back1 = (Triangle){back_vertices[0], back_vertices[1], back_vertices[2]};

		return 3;
	} else if (coplanar == 1 && front_count == 1 && back_count == 1) {
		// this is an edge case where a triangle cuts right through the plane with it's central point
		// on the plane. Currently I don't handle this case as the input data doesn't require it - and
		// soon BSP's will be loaded automatically after being compiled externally by a more effective
		// tool written in C++.
	} else {
		return 0;
	}
}

Vertex calculate_center(Vertex v0, Vertex v1, Vertex v2) {
	Vertex center;
	center.x = (v0.x + v1.x + v2.x) / 3.0f;
	center.y = (v0.y + v1.y + v2.y) / 3.0f;
	center.z = (v0.z + v1.z + v2.z) / 3.0f;
	center.u = (v0.u + v1.u + v2.u) / 3.0f;
	center.v = (v0.v + v1.v + v2.v) / 3.0f;
	center.color = (v0.color + v1.color + v2.color) / 3;
	center.alpha = (v0.alpha + v1.alpha + v2.alpha) / 3.0f;
	return center;
}

void enclose_aabb(AABB *aabb1, const AABB *aabb2) {

	if (aabb2->min.x < aabb1->min.x) {
		aabb1->min.x = aabb2->min.x;
	}
	if (aabb2->min.y < aabb1->min.y) {
		aabb1->min.y = aabb2->min.y;
	}
	if (aabb2->min.z < aabb1->min.z) {
		aabb1->min.z = aabb2->min.z;
	}


	if (aabb2->max.x > aabb1->max.x) {
		aabb1->max.x = aabb2->max.x;
	}
	if (aabb2->max.y > aabb1->max.y) {
		aabb1->max.y = aabb2->max.y;
	}
	if (aabb2->max.z > aabb1->max.z) {
		aabb1->max.z = aabb2->max.z;
	}
}

BSPNode *build_bsp_tree(Triangle *triangles, int count) {
	if (count == 0) return NULL;

	int best_split_index = 0;
	int min_split_count = count * count;

	for (int i = 0; i < count; i++) {
		float nx, ny, nz;
		calculate_normal(triangles[i].v0, triangles[i].v1, triangles[i].v2, &nx, &ny, &nz);

		int front_count = 0, back_count = 0, split_count = 0;
		for (int j = 0; j < count; j++) {
			if (i == j) continue;
			Triangle front1, front2, back1, back2;
			int result = split_triangle(triangles[j], triangles[i].v0, nx, ny, nz, &front1, &front2, &back1, &back2);
			if (result == 2 || result == 3) {
				split_count++;
			} else {

				int side[3];
				side[0] = classify_vertex(triangles[j].v0, triangles[i].v0, nx, ny, nz);
				side[1] = classify_vertex(triangles[j].v1, triangles[i].v0, nx, ny, nz);
				side[2] = classify_vertex(triangles[j].v2, triangles[i].v0, nx, ny, nz);
				if (side[0] > 0.0001f && side[1] > 0.0001f && side[2] > 0.0001f) {
					front_count++;
				} else if (side[0] < -0.0001f && side[1] < -0.0001f && side[2] < -0.0001f) {
					back_count++;
				} else {
					Vertex center = interpolate_vertex(triangles[j].v0, triangles[j].v1, 0.5f);
					center = interpolate_vertex(center, triangles[j].v2, 0.5f);

					int s = classify_vertex(center, triangles[i].v0, nx, ny, nz);

					if (s > 0.0001f) {
						front_count++;
					} else if (s < -0.0001f) {
						back_count++;
					} else {
						front_count++;
					}
				}
			}
		}

		int imbalance = abs(front_count - back_count);
		int split_score = split_count + imbalance;
		if (split_score < min_split_count) {
			best_split_index = i;
			min_split_count = split_score;
		}
	}


	BSPNode *node = (BSPNode *) malloc(sizeof(BSPNode));
	memset(node, 0, sizeof(BSPNode));
	node->triangle = triangles[best_split_index];
	node->front = node->back =
			NULL;


	calculate_aabb(&node->triangle, &node->bounding_box);

	if (count == 1) return node;

	float nx, ny, nz;
	calculate_normal(node->triangle.v0, node->triangle.v1, node->triangle.v2, &nx, &ny, &nz);

	Triangle *front_list = (Triangle *) malloc(count * sizeof(Triangle));
	Triangle *back_list = (Triangle *) malloc(count * sizeof(Triangle));

	int front_count = 0, back_count = 0;

	for (int i = 0; i < count; i++) {
		if (i == best_split_index) continue;
		Triangle front1, front2, back1, back2;
		int result = split_triangle(triangles[i], node->triangle.v0, nx, ny, nz, &front1, &front2, &back1, &back2);
		if (result == 2) {
			continue;
			front_list[front_count++] = front1;
			back_list[back_count++] = back1;
			back_list[back_count++] = back2;
		} else if (result == 3) {
			continue;
			front_list[front_count++] = front1;
			front_list[front_count++] = front2;
			back_list[back_count++] = back1;
		} else {

			int side[3];
			side[0] = classify_vertex(triangles[i].v0, node->triangle.v0, nx, ny, nz);
			side[1] = classify_vertex(triangles[i].v1, node->triangle.v0, nx, ny, nz);
			side[2] = classify_vertex(triangles[i].v2, node->triangle.v0, nx, ny, nz);
			if (side[0] > 0.0001f && side[1] > 0.0001f && side[2] > 0.0001f) {
				front_list[front_count++] = triangles[i];
			} else if (side[0] < -0.0001f && side[1] < -0.0001f && side[2] < -0.0001f) {
				back_list[back_count++] = triangles[i];
			} else {


				Vertex center = interpolate_vertex(triangles[i].v0, triangles[i].v1, 0.5f);
				center = interpolate_vertex(center, triangles[i].v2, 0.5f);


				int s = classify_vertex(center, node->triangle.v0, nx, ny, nz);

				if (s > 0.0001f) {
					front_list[front_count++] = triangles[i];
				} else if (s < -0.0001f) {
					back_list[back_count++] = triangles[i];
				} else {
					front_list[front_count++] = triangles[i];
				}
			}
		}
	}

	node->front = build_bsp_tree(front_list, front_count);
	node->back = build_bsp_tree(back_list, back_count);

	free(front_list);
	free(back_list);


	if (node->back != NULL) {
		enclose_aabb(&node->bounding_box, &node->back->bounding_box);
	}

	if (node->front != NULL) {
		enclose_aabb(&node->bounding_box, &node->front->bounding_box);
	}

	return node;
}


void free_bsp_tree(BSPNode *node) {
	if (node == NULL) return;

	free_bsp_tree(node->front);
	free_bsp_tree(node->back);

	free(node);
}
