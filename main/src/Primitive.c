
// Written by Lynton "Pionwave" Schneider, 2024

#include <math.h>
#include <string.h>


#include "Primitive.h"

Vertex transform_vertex(Vertex v, Vertex camera, float camera_yaw) {
	float tx = v.x - camera.x;
	float ty = v.y - camera.y;
	float tz = v.z - camera.z;

	float cos_yaw = cos(camera_yaw);
	float sin_yaw = sin(camera_yaw);

	float px = cos_yaw * tx - sin_yaw * tz;
	float py = ty;
	float pz = sin_yaw * tx + cos_yaw * tz;

	return (Vertex){px, py, pz, v.u, v.v, v.color, v.alpha};
}

void create_projection_matrix(Matrix4 *proj, float screen_width, float screen_height, float fovd, float near, float far) {
	float fovr = fovd * (M_PI / 180.0f);
	float top = tan(fovr * 0.5f) * near;
	float bottom = -top;
	float aspect = (float) screen_width / (float) screen_height;
	float right = top * aspect;
	float left = -right;

	float A = (right + left) / (right - left);
	float B = (top + bottom) / (top - bottom);
	float C = -((far + near) / (far - near));
	float D = -((2 * far * near) / (far - near));

	Matrix4 projection;

	memset(projection, 0, sizeof(Matrix4));

	projection[0][0] = (2 * near) / (right - left);
	projection[0][2] = A;

	projection[1][1] = (2 * near) / (top - bottom);
	projection[2][1] = B;

	projection[2][2] = C;
	projection[3][2] = D;

	projection[2][3] = -1;

	projection[3][3] = 0;

	memcpy(proj, &projection, sizeof(Matrix4));
}

void project(Vertex v, Matrix4 projMatrix, int *screen_x, int *screen_y, float *screen_z, float *screen_w) {
	float screen_width = (float) 320;
	float screen_height = (float) 200;


	float clipX = v.x * projMatrix[0][0] + v.y * projMatrix[1][0] + v.z * projMatrix[2][0] + projMatrix[3][0];
	float clipY = v.x * projMatrix[0][1] + v.y * projMatrix[1][1] + v.z * projMatrix[2][1] + projMatrix[3][1];
	float clipZ = v.x * projMatrix[0][2] + v.y * projMatrix[1][2] + v.z * projMatrix[2][2] + projMatrix[3][2];
	float clipW = v.x * projMatrix[0][3] + v.y * projMatrix[1][3] + v.z * projMatrix[2][3] + projMatrix[3][3];


	if (clipW != 0.0f) {
		clipX /= clipW;
		clipY /= clipW;
		clipZ /= clipW;
	}


	*screen_x = (int) ((clipX + 1.0f) * 0.5f * screen_width);
	*screen_y = (int) ((clipY + 1.0f) * 0.5f * screen_height);
	*screen_z = (clipZ + 1.0f) * 0.5f;
	*screen_w = clipW;
}

int is_triangle_in_front(Triangle tri) {
	return (tri.v0.z > 0 && tri.v1.z > 0 && tri.v2.z > 0);
}

void calculate_normal(Vertex v0, Vertex v1, Vertex v2, float *nx, float *ny, float *nz) {
	float ux = v1.x - v0.x;
	float uy = v1.y - v0.y;
	float uz = v1.z - v0.z;

	float vx = v2.x - v0.x;
	float vy = v2.y - v0.y;
	float vz = v2.z - v0.z;

	*nx = uy * vz - uz * vy;
	*ny = uz * vx - ux * vz;
	*nz = ux * vy - uy * vx;
}

Plane calculatePlane(Triangle triangle) {
	Vertex v0 = triangle.v0;
	Vertex v1 = triangle.v1;
	Vertex v2 = triangle.v2;


	float ux = v1.x - v0.x;
	float uy = v1.y - v0.y;
	float uz = v1.z - v0.z;

	float vx = v2.x - v0.x;
	float vy = v2.y - v0.y;
	float vz = v2.z - v0.z;


	float A = uy * vz - uz * vy;
	float B = uz * vx - ux * vz;
	float C = ux * vy - uy * vx;


	float D = -(A * v0.x + B * v0.y + C * v0.z);

	Plane plane = {A, B, C, D};
	return plane;
}

Vertex interpolate_vertex(Vertex v1, Vertex v2, float t) {
	return (Vertex){
			v1.x + t * (v2.x - v1.x),
			v1.y + t * (v2.y - v1.y),
			v1.z + t * (v2.z - v1.z),
			v1.u + t * (v2.u - v1.u),
			v1.v + t * (v2.v - v1.v),
			(unsigned char) (v1.color + t * (v2.color - v1.color)),
			v1.alpha + t * (v2.alpha - v1.alpha)};
}


void calculate_plane(Plane *plane, Vertex p0, Vertex p1, Vertex p2) {
	Vertex v1, v2, normal;

	v1.x = p1.x - p0.x;
	v1.y = p1.y - p0.y;
	v1.z = p1.z - p0.z;

	v2.x = p2.x - p0.x;
	v2.y = p2.y - p0.y;
	v2.z = p2.z - p0.z;

	normal = cross_product(v1, v2);
	normalize(&normal);

	plane->A = normal.x;
	plane->B = normal.y;
	plane->C = normal.z;
	plane->D = -(normal.x * p0.x + normal.y * p0.y + normal.z * p0.z);
}

void create_camera_frustum(CameraFrustum *frustum, Vertex camera_pos, Vertex forward_vec, Vertex up_vec, float fov, float aspect_ratio, float near_dist, float far_dist) {
	Vertex right_vec, near_center, far_center, near_top_left, near_top_right, near_bottom_left, near_bottom_right, far_top_left, far_top_right, far_bottom_left, far_bottom_right;


	right_vec = cross_product(forward_vec, up_vec);
	normalize(&right_vec);


	near_center.x = camera_pos.x + forward_vec.x * near_dist;
	near_center.y = camera_pos.y + forward_vec.y * near_dist;
	near_center.z = camera_pos.z + forward_vec.z * near_dist;

	far_center.x = camera_pos.x + forward_vec.x * far_dist;
	far_center.y = camera_pos.y + forward_vec.y * far_dist;
	far_center.z = camera_pos.z + forward_vec.z * far_dist;

	float fov_rad = fov * (M_PI / 180.0f);


	float near_height = 2.0f * tan(fov_rad / 2.0f) * near_dist;
	float near_width = near_height * aspect_ratio;
	float far_height = 2.0f * tan(fov_rad / 2.0f) * far_dist;
	float far_width = far_height * aspect_ratio;


	near_top_left.x = near_center.x + up_vec.x * (near_height / 2.0f) - right_vec.x * (near_width / 2.0f);
	near_top_left.y = near_center.y + up_vec.y * (near_height / 2.0f) - right_vec.y * (near_width / 2.0f);
	near_top_left.z = near_center.z + up_vec.z * (near_height / 2.0f) - right_vec.z * (near_width / 2.0f);

	near_top_right.x = near_center.x + up_vec.x * (near_height / 2.0f) + right_vec.x * (near_width / 2.0f);
	near_top_right.y = near_center.y + up_vec.y * (near_height / 2.0f) + right_vec.y * (near_width / 2.0f);
	near_top_right.z = near_center.z + up_vec.z * (near_height / 2.0f) + right_vec.z * (near_width / 2.0f);

	near_bottom_left.x = near_center.x - up_vec.x * (near_height / 2.0f) - right_vec.x * (near_width / 2.0f);
	near_bottom_left.y = near_center.y - up_vec.y * (near_height / 2.0f) - right_vec.y * (near_width / 2.0f);
	near_bottom_left.z = near_center.z - up_vec.z * (near_height / 2.0f) - right_vec.z * (near_width / 2.0f);

	near_bottom_right.x = near_center.x - up_vec.x * (near_height / 2.0f) + right_vec.x * (near_width / 2.0f);
	near_bottom_right.y = near_center.y - up_vec.y * (near_height / 2.0f) + right_vec.y * (near_width / 2.0f);
	near_bottom_right.z = near_center.z - up_vec.z * (near_height / 2.0f) + right_vec.z * (near_width / 2.0f);


	far_top_left.x = far_center.x + up_vec.x * (far_height / 2.0f) - right_vec.x * (far_width / 2.0f);
	far_top_left.y = far_center.y + up_vec.y * (far_height / 2.0f) - right_vec.y * (far_width / 2.0f);
	far_top_left.z = far_center.z + up_vec.z * (far_height / 2.0f) - right_vec.z * (far_width / 2.0f);

	far_top_right.x = far_center.x + up_vec.x * (far_height / 2.0f) + right_vec.x * (far_width / 2.0f);
	far_top_right.y = far_center.y + up_vec.y * (far_height / 2.0f) + right_vec.y * (far_width / 2.0f);
	far_top_right.z = far_center.z + up_vec.z * (far_height / 2.0f) + right_vec.z * (far_width / 2.0f);

	far_bottom_left.x = far_center.x - up_vec.x * (far_height / 2.0f) - right_vec.x * (far_width / 2.0f);
	far_bottom_left.y = far_center.y - up_vec.y * (far_height / 2.0f) - right_vec.y * (far_width / 2.0f);
	far_bottom_left.z = far_center.z - up_vec.z * (far_height / 2.0f) - right_vec.z * (far_width / 2.0f);

	far_bottom_right.x = far_center.x - up_vec.x * (far_height / 2.0f) + right_vec.x * (far_width / 2.0f);
	far_bottom_right.y = far_center.y - up_vec.y * (far_height / 2.0f) + right_vec.y * (far_width / 2.0f);
	far_bottom_right.z = far_center.z - up_vec.z * (far_height / 2.0f) + right_vec.z * (far_width / 2.0f);


	calculate_plane(&frustum->nearPlane, near_top_right, near_top_left, near_bottom_right);
	calculate_plane(&frustum->farPlane, far_top_right, far_top_left, far_bottom_left);
	calculate_plane(&frustum->leftPlane, near_top_left, far_top_left, far_bottom_left);
	calculate_plane(&frustum->rightPlane, far_top_right, near_top_right, near_bottom_right);
	calculate_plane(&frustum->topPlane, near_top_left, far_top_left, far_top_right);
	calculate_plane(&frustum->bottomPlane, near_bottom_left, far_bottom_left, far_bottom_right);
}

float signed_distance_to_plane(Plane *plane, Vertex *vert) {
	return (plane->A * vert->x + plane->B * vert->y + plane->C * vert->z + plane->D) / sqrt(plane->A * plane->A + plane->B * plane->B + plane->C * plane->C);
}

int is_on_or_forward_plane(Plane *plane, Sphere *sphere) {


	Vertex normal;
	normal.x = plane->A;
	normal.y = plane->B;
	normal.z = plane->C;

	float dot_distance = dot_product(normal, sphere->center) + plane->D;

	if (dot_distance < sphere->radius) {
		return 0;
	}

	return 1;
}

Sphere calculate_bounding_sphere_from_aabb(Vertex min, Vertex max) {
	Sphere sphere;


	sphere.center.x = (min.x + max.x) / 2.0f;
	sphere.center.y = (min.y + max.y) / 2.0f;
	sphere.center.z = (min.z + max.z) / 2.0f;


	float dx = max.x - sphere.center.x;
	float dy = max.y - sphere.center.y;
	float dz = max.z - sphere.center.z;
	sphere.radius = sqrt((dx * dx) + (dy * dy) + (dz * dz));

	return sphere;
}


void create_cone_frustum(ConeFrustum *frustum, Vertex player_pos, Vertex forward_vec, float fov, float far_plane_distance, float near_plane_distance) {

	float half_fov_rad = (fov / 2.0f) * (M_PI / 180.0f);
	float fov_rad = (fov / 2.0f) * (M_PI / 180.0f);


	frustum->cos_fov = cos(fov_rad);
	frustum->sin_fov = sin(fov_rad);


	float aspect_ratio = 320.0f / 200.0f;


	float diagonal_fov_rad = atan(tan(half_fov_rad) * sqrt(1 + aspect_ratio * aspect_ratio));


	frustum->radius = far_plane_distance * tan(diagonal_fov_rad);


	frustum->apex.x = player_pos.x - (forward_vec.x * near_plane_distance);
	frustum->apex.y = player_pos.y - (forward_vec.y * near_plane_distance);
	frustum->apex.z = player_pos.z - (forward_vec.z * near_plane_distance);


	frustum->direction = forward_vec;
	frustum->height = far_plane_distance;
}
