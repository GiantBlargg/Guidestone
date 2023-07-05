#pragma once

#include "types.hpp"

struct Vector2 {
	f32 x, y;
};
template <typename T> T& operator>>(T& t, Vector2& v) { return t >> v.x >> v.y; }

struct Vector3 {
	f32 x, y, z;
};
template <typename T> T& operator>>(T& t, Vector3& v) { return t >> v.x >> v.y >> v.z; }

struct Matrix4 {
	f32 m11, m21, m31, m41, m12, m22, m32, m42, m13, m23, m33, m43, m14, m24, m34, m44;
};
Matrix4 operator*(Matrix4& a, Matrix4& b);
template <typename T> T& operator>>(T& t, Matrix4& m) {
	return t >> m.m11 >> m.m21 >> m.m31 >> m.m41 >> m.m12 >> m.m22 >> m.m32 >> m.m42 >> m.m13 >> m.m23 >> m.m33 >>
		m.m43 >> m.m14 >> m.m24 >> m.m34 >> m.m44;
}
