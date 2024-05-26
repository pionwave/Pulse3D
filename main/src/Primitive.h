
// Written by Lynton "Pionwave" Schneider, 2024

#ifndef __PULSE_PRIMITIVE_H__
#define __PULSE_PRIMITIVE_H__

#include "Math.h"

typedef struct {
	float x, y, z;
	float u, v;
	unsigned char color;
	float alpha;
} Vertex;


typedef struct {
	Vertex v0, v1, v2;
} Triangle;

typedef struct {
	float A, B, C, D;
} Plane;


typedef struct {
	Vertex apex;
	Vertex direction;


	float sin_fov;
	float cos_fov;
	float radius;
	float height;
} ConeFrustum;

typedef struct {
	Plane nearPlane;
	Plane farPlane;
	Plane leftPlane;
	Plane rightPlane;
	Plane topPlane;
	Plane bottomPlane;
} CameraFrustum;

typedef struct {
	Vertex center;
	float radius;
} Sphere;

typedef struct {
	Vertex center;
	float radius_x;
	float radius_y;
	float radius_z;
} Ellipsoid;


typedef float Matrix4[4][4];

Vertex transform_vertex(Vertex v, Vertex camera, float camera_yaw);
void create_projection_matrix(Matrix4 *proj, float screen_width, float screen_height, float fovd, float near, float far);

void project(Vertex v, Matrix4 projMatrix, int *screen_x, int *screen_y, float *screen_z, float *screen_w);

void calculate_normal(Vertex v0, Vertex v1, Vertex v2, float *nx, float *ny, float *nz);
Vertex interpolate_vertex(Vertex v1, Vertex v2, float t);
Plane calculatePlane(Triangle triangle);

inline float dot_product(Vertex v1, Vertex v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Vertex subtract(Vertex v1, Vertex v2) {
	Vertex result = {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
	return result;
}

inline Vertex multiply(Vertex v, float scalar) {
	Vertex result = {v.x * scalar, v.y * scalar, v.z * scalar};
	return result;
}

inline Vertex add(Vertex v1, Vertex v2) {
	Vertex result = {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
	return result;
}

inline void normalize(Vertex *v) {
	float length = fast_sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
	v->x /= length;
	v->y /= length;
	v->z /= length;
}

inline Vertex cross_product(Vertex v1, Vertex v2) {
	Vertex result;
	result.x = v1.y * v2.z - v1.z * v2.y;
	result.y = v1.z * v2.x - v1.x * v2.z;
	result.z = v1.x * v2.y - v1.y * v2.x;
	return result;
}

void create_cone_frustum(ConeFrustum *frustum, Vertex player_pos, Vertex forward_vec, float fov, float far_plane_distance, float near_plane_distance);

Sphere calculate_bounding_sphere_from_aabb(Vertex min, Vertex max);
float signed_distance_to_plane(Plane *plane, Vertex *vert);
int is_on_or_forward_plane(Plane *plane, Sphere *sphere);
void calculate_plane(Plane *plane, Vertex p0, Vertex p1, Vertex p2);
void create_camera_frustum(CameraFrustum *frustum, Vertex camera_pos, Vertex forward_vec, Vertex up_vec, float fov, float aspect_ratio, float near_dist, float far_dist);

#endif