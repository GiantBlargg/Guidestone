#include "math.hpp"
#include <cmath>
#include <numbers>

const Matrix4 Matrix4::identity = {{
	{1, 0, 0, 0},
	{0, 1, 0, 0},
	{0, 0, 1, 0},
	{0, 0, 0, 1},
}};

Matrix4 Matrix4::perspective(f64 fovy, f64 aspect, f64 zNear) {
	f64 radians = fovy * std::numbers::pi / 360.0;
	f64 cot = cos(radians) / sin(radians);

	Matrix4 m;
	m[0][0] = -cot / aspect;
	m[1][1] = cot;
	m[2][3] = 1.0f;
	m[3][2] = -zNear;

	return m;
}
