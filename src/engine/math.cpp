#include "math.hpp"
#include <cassert>
#include <cmath>
#include <numbers>

const Matrix4 Matrix4::identity = {
	{1, 0, 0, 0},
	{0, 1, 0, 0},
	{0, 0, 1, 0},
	{0, 0, 0, 1},
};

Matrix4 Matrix4::lookAt(Vector3 eye, Vector3 center, Vector3 up) {
	Vector3 backwards = normalized(eye - center);
	Vector3 side = normalized(cross(up, backwards));
	up = normalized(cross(backwards, side));

	return {
		{side.x, up.x, backwards.x, 0},
		{side.y, up.y, backwards.y, 0},
		{side.z, up.z, backwards.z, 0},
		{-eye * side, -eye * up, -eye * backwards, 1},
	};
}

Matrix4 Matrix4::perspective(f32 fovy, f32 aspect, f32 zNear) {
	f32 halfFov = fovy / 2;
	f32 cot = cos(halfFov) / sin(halfFov);

	Matrix4 m;
	m[0][0] = -cot / aspect;
	m[1][1] = cot;
	m[2][3] = 1.0f;
	m[3][2] = -zNear;

	return m;
}
