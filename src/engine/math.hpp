#pragma once

#include "types.hpp"
#include <cmath>
#include <numbers>

consteval f32 degToRad(f32 deg) { return deg * std::numbers::pi / 360.0; }
consteval f64 degToRad(f64 deg) { return deg * std::numbers::pi / 360.0; }

// All angles are radians

struct Vector2 {
	f32 x = 0, y = 0;

	f32& operator[](int axis) { return ((f32*)this)[axis]; }
};

inline Vector2 operator-(const Vector2& v) { return {-v.x, -v.y}; }
inline Vector2 operator*(const Vector2& v, f32 s) { return {v.x * s, v.y * s}; }
inline Vector2 operator/(const Vector2& v, f32 s) { return {v.x / s, v.y / s}; }
inline Vector2 operator+(const Vector2& a, const Vector2& b) { return {a.x + b.x, a.y + b.y}; }
inline Vector2 operator-(const Vector2& a, const Vector2& b) { return {a.x - b.x, a.y - b.y}; }
inline f32 operator*(const Vector2& a, const Vector2 b) { return a.x * b.x + a.y * b.y; }

template <typename T> T& operator>>(T& t, Vector2& v) { return t >> v.x >> v.y; }

struct Vector3 {
	f32 x = 0, y = 0, z = 0;

	f32& operator[](int axis) { return ((f32*)this)[axis]; }
};

inline Vector3 operator-(const Vector3& v) { return {-v.x, -v.y, -v.z}; }
inline Vector3 operator*(const Vector3& v, f32 s) { return {v.x * s, v.y * s, v.z * s}; }
inline Vector3 operator/(const Vector3& v, f32 s) { return {v.x / s, v.y / s, v.z / s}; }
inline Vector3 operator+(const Vector3& a, const Vector3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vector3 operator-(const Vector3& a, const Vector3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

// Dot product
inline f32 operator*(const Vector3& a, const Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vector3 cross(const Vector3& a, const Vector3& b) {
	return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

template <typename T> T& operator>>(T& t, Vector3& v) { return t >> v.x >> v.y >> v.z; }

struct Vector4 {
	f32 x = 0, y = 0, z = 0, w = 0;

	f32& operator[](int axis) { return ((f32*)this)[axis]; }
};

inline Vector4 operator-(const Vector4& v) { return {-v.x, -v.y, -v.z, -v.w}; }
inline Vector4 operator*(const Vector4& v, f32 s) { return {v.x * s, v.y * s, v.z * s, v.w * s}; }
inline Vector4 operator/(const Vector4& v, f32 s) { return {v.x / s, v.y / s, v.z / s, v.w / s}; }
inline Vector4 operator+(const Vector4& a, const Vector4& b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Vector4 operator-(const Vector4& a, const Vector4& b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
inline f32 operator*(const Vector4& a, const Vector4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
template <typename T> T& operator>>(T& t, Vector4& v) { return t >> v.x >> v.y >> v.z >> v.w; }

inline f32 lengthSquared(const Vector2& v) { return v.x * v.x + v.y * v.y; }
inline f32 lengthSquared(const Vector3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
inline f32 lengthSquared(const Vector4& v) { return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w; }
template <typename V> f32 length(const V& v) { return sqrt(lengthSquared(v)); }
template <typename V> V normalized(const V& v) {
	auto mag = length(v);
	if (mag == 0) [[unlikely]] {
		return v;
	}
	return v / mag;
}

struct Matrix4 {
	Vector4 columns[4];

	Matrix4() = default;
	Matrix4(Vector4 c0, Vector4 c1, Vector4 c2, Vector4 c3) : columns{c0, c1, c2, c3} {}

	Vector4& operator[](int c) { return columns[c]; }
	const Vector4& operator[](int c) const { return columns[c]; }

	const static Matrix4 identity;

	static Matrix4 translate(Vector3 v) {
		Matrix4 m = identity;
		m[3].x = v.x;
		m[3].y = v.y;
		m[3].z = v.z;
		return m;
	}

	static Matrix4 lookAt(Vector3 eye, Vector3 center, Vector3 up);

	// This is a little special
	// It maps from OpenGL camera space to Vulkan NDC with reversed depth and infinite zFar
	static Matrix4 perspective(f32 fovy, f32 aspect, f32 zNear);
};
inline Vector4 operator*(const Matrix4& m, const Vector4& v) {
	return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
}
inline Matrix4 operator*(const Matrix4& a, const Matrix4& b) { return {a * b[0], a * b[1], a * b[2], a * b[3]}; }
template <typename T> T& operator>>(T& t, Matrix4& m) { return t >> m[0] >> m[1] >> m[2] >> m[3]; }
