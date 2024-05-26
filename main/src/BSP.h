
// Written by Lynton "Pionwave" Schneider, 2024

#ifndef __PULSE_BSP_H__
#define __PULSE_BSP_H__

#include "Primitive.h"

typedef struct {
	Vertex min;
	Vertex max;
} AABB;

typedef struct BSPNode {
	Triangle triangle;
	Plane split_plane;
	AABB bounding_box;
	struct BSPNode *front;
	struct BSPNode *back;
} BSPNode;

BSPNode *build_bsp_tree(Triangle *triangles, int count);
void free_bsp_tree(BSPNode *node);

void enclose_aabb(AABB *aabb1, const AABB *aabb2);


#endif
