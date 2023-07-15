#pragma once

#include "types.hpp"

struct Vector2 {
	f32 x = 0, y = 0;

	f32& operator[](int axis) { return ((f32*)this)[axis]; }
};
inline Vector2 operator+(const Vector2& a, const Vector2& b) { return Vector2{a.x + b.x, a.y + b.y}; }
inline Vector2 operator*(const Vector2& v, f32 s) { return Vector2{v.x * s, v.y * s}; }
inline Vector2 operator/(const Vector2& v, f32 s) { return Vector2{v.x / s, v.y / s}; }
template <typename T> T& operator>>(T& t, Vector2& v) { return t >> v.x >> v.y; }

struct Vector3 {
	f32 x = 0, y = 0, z = 0;

	f32& operator[](int axis) { return ((f32*)this)[axis]; }
};
inline Vector3 operator+(const Vector3& a, const Vector3& b) { return Vector3{a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vector3 operator*(const Vector3& v, f32 s) { return Vector3{v.x * s, v.y * s, v.z * s}; }
inline Vector3 operator/(const Vector3& v, f32 s) { return Vector3{v.x / s, v.y / s, v.z / s}; }
template <typename T> T& operator>>(T& t, Vector3& v) { return t >> v.x >> v.y >> v.z; }

struct Vector4 {
	f32 x = 0, y = 0, z = 0, w = 0;

	f32& operator[](int axis) { return ((f32*)this)[axis]; }
};
inline Vector4 operator+(const Vector4& a, const Vector4& b) {
	return Vector4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}
inline Vector4 operator*(const Vector4& v, f32 s) { return Vector4{v.x * s, v.y * s, v.z * s, v.w * s}; }
inline Vector4 operator/(const Vector4& v, f32 s) { return Vector4{v.x / s, v.y / s, v.z / s, v.w / s}; }
template <typename T> T& operator>>(T& t, Vector4& v) { return t >> v.x >> v.y >> v.z >> v.w; }

struct Matrix4 {
	Vector4 columns[4];

	Vector4& operator[](int c) { return columns[c]; }
	const Vector4& operator[](int c) const { return columns[c]; }

	const static Matrix4 identity;

	// This is a little special
	// It maps from OpenGL camera space to Vulkan NDC with reversed depth and infinite zFar
	static Matrix4 perspective(f64 fovy, f64 aspect, f64 zNear);
};
inline Vector4 operator*(const Matrix4& m, const Vector4& v) {
	return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
}
inline Matrix4 operator*(const Matrix4& a, const Matrix4& b) {
	return Matrix4{{a * b[0], a * b[1], a * b[2], a * b[3]}};
}
template <typename T> T& operator>>(T& t, Matrix4& m) { return t >> m[0] >> m[1] >> m[2] >> m[3]; }
