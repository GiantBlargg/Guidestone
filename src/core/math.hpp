#pragma once

#include "types.hpp"
#include <cmath>
#include <numbers>

template <typename T> consteval T degToRad(T deg) {
	static_assert(std::is_floating_point_v<T>);
	return deg * std::numbers::pi / 180.0;
}

// All angles are radians

template <typename T = f32> struct Vector2 {
	static_assert(std::is_arithmetic_v<T>);

	T x = 0, y = 0;

	T& operator[](int axis) { return ((T*)this)[axis]; }

	friend inline Vector2 operator-(const Vector2& v) { return {-v.x, -v.y}; }

	friend inline Vector2 operator*(const Vector2& v, T s) { return {v.x * s, v.y * s}; }
	friend inline Vector2 operator/(const Vector2& v, T s) { return {v.x / s, v.y / s}; }
	friend inline Vector2 operator+(const Vector2& a, const Vector2& b) { return {a.x + b.x, a.y + b.y}; }
	friend inline Vector2 operator-(const Vector2& a, const Vector2& b) { return {a.x - b.x, a.y - b.y}; }

	friend inline Vector2 operator*=(Vector2& v, T s) { return {v.x *= s, v.y *= s}; }
	friend inline Vector2 operator/=(Vector2& v, T s) { return {v.x /= s, v.y /= s}; }
	friend inline Vector2 operator+=(Vector2& a, const Vector2& b) { return {a.x += b.x, a.y += b.y}; }
	friend inline Vector2 operator-=(Vector2& a, const Vector2& b) { return {a.x -= b.x, a.y -= b.y}; }

	friend inline T operator*(const Vector2& a, const Vector2 b) { return a.x * b.x + a.y * b.y; }

	friend inline T lengthSquared(const Vector2& v) { return v.x * v.x + v.y * v.y; }

	template <typename R> friend R& operator>>(R& r, Vector2& v) { return r >> v.x >> v.y; }
};

using vec2 = Vector2<f32>;
using uvec2 = Vector2<u32>;

template <typename T = f32> struct Vector3 {
	static_assert(std::is_arithmetic_v<T>);

	T x = 0, y = 0, z = 0;

	T& operator[](int axis) { return ((T*)this)[axis]; }

	friend inline Vector3 operator-(const Vector3& v) { return {-v.x, -v.y, -v.z}; }

	friend inline Vector3 operator*(const Vector3& v, T s) { return {v.x * s, v.y * s, v.z * s}; }
	friend inline Vector3 operator/(const Vector3& v, T s) { return {v.x / s, v.y / s, v.z / s}; }
	friend inline Vector3 operator+(const Vector3& a, const Vector3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
	friend inline Vector3 operator-(const Vector3& a, const Vector3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

	friend inline Vector3 operator*=(Vector3& v, T s) { return {v.x *= s, v.y *= s, v.z *= s}; }
	friend inline Vector3 operator/=(Vector3& v, T s) { return {v.x /= s, v.y /= s, v.z /= s}; }
	friend inline Vector3 operator+=(Vector3& a, const Vector3& b) { return {a.x += b.x, a.y += b.y, a.z += b.z}; }
	friend inline Vector3 operator-=(Vector3& a, const Vector3& b) { return {a.x -= b.x, a.y -= b.y, a.z -= b.z}; }

	// Dot product
	friend inline T operator*(const Vector3& a, const Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
	friend inline Vector3 cross(const Vector3& a, const Vector3& b) {
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	}

	friend inline T lengthSquared(const Vector3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }

	template <typename R> friend R& operator>>(R& r, Vector3& v) { return r >> v.x >> v.y >> v.z; }
};

using fvec3 = Vector3<f32>;
using u8vec3 = Vector3<u8>;
using vec3 = fvec3;

template <typename T = f32> struct Vector4 {
	static_assert(std::is_arithmetic_v<T>);

	T x = 0, y = 0, z = 0, w = 0;

	T& operator[](int axis) { return ((T*)this)[axis]; }

	friend inline Vector4 operator-(const Vector4& v) { return {-v.x, -v.y, -v.z, -v.w}; }

	friend inline Vector4 operator*(const Vector4& v, T s) { return {v.x * s, v.y * s, v.z * s, v.w * s}; }
	friend inline Vector4 operator/(const Vector4& v, T s) { return {v.x / s, v.y / s, v.z / s, v.w / s}; }
	friend inline Vector4 operator+(const Vector4& a, const Vector4& b) {
		return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
	}
	friend inline Vector4 operator-(const Vector4& a, const Vector4& b) {
		return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
	}

	friend inline Vector4 operator*=(Vector4& v, T s) { return {v.x *= s, v.y *= s, v.z *= s, v.w *= s}; }
	friend inline Vector4 operator/=(Vector4& v, T s) { return {v.x /= s, v.y /= s, v.z /= s, v.w /= s}; }
	friend inline Vector4 operator+=(Vector4& a, const Vector4& b) {
		return {a.x += b.x, a.y += b.y, a.z += b.z, a.w += b.w};
	}
	friend inline Vector4 operator-=(Vector4& a, const Vector4& b) {
		return {a.x -= b.x, a.y -= b.y, a.z -= b.z, a.w -= b.w};
	}

	friend inline T operator*(const Vector4& a, const Vector4 b) {
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	friend inline T lengthSquared(const Vector4& v) { return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w; }

	template <typename R> friend R& operator>>(R& r, Vector4& v) { return r >> v.x >> v.y >> v.z >> v.w; }
};

using fvec4 = Vector4<f32>;
using u8vec4 = Vector4<u8>;
using vec4 = fvec4;

template <typename V> auto length(const V& v) { return sqrt(lengthSquared(v)); }
template <typename V> V normalized(const V& v) {
	auto mag = length(v);
	if (mag == 0) [[unlikely]] {
		return v;
	}
	return v / mag;
}

template <typename T = f32> struct Matrix3 {
	static_assert(std::is_floating_point_v<T>);

	using Vector = Vector3<T>;

	Vector columns[3];

	Matrix3() = default;
	Matrix3(Vector c0, Vector c1, Vector c2) : columns{c0, c1, c2} {}

	Vector& operator[](int c) { return columns[c]; }
	const Vector& operator[](int c) const { return columns[c]; }

	static Matrix3 identity() {
		return {
			{1, 0, 0},
			{0, 1, 0},
			{0, 0, 1},
		};
	}

	friend inline Vector operator*(const Matrix3& m, const Vector& v) { return m[0] * v.x + m[1] * v.y + m[2] * v.z; }

	friend inline Matrix3 operator*(const Matrix3& a, const Matrix3& b) { return {a * b[0], a * b[1], a * b[2]}; }
	friend inline Matrix3 operator*=(Matrix3& a, const Matrix3& b) { return {a *= b[0], a *= b[1], a *= b[2]}; }

	template <typename R> friend R& operator>>(R& r, Matrix3& m) { return r >> m[0] >> m[1] >> m[2] >> m[3]; }

	static Matrix3 rotateX(T angle) {
		const T cos_angle = cos(angle);
		const T sin_angle = sin(angle);
		return {
			{1, 0, 0},
			{0, cos_angle, sin_angle},
			{0, -sin_angle, cos_angle},
		};
	}
	static Matrix3 rotateY(T angle) {
		const T cos_angle = cos(angle);
		const T sin_angle = sin(angle);
		return {
			{cos_angle, 0, -sin_angle},
			{0, 1, 0},
			{sin_angle, 0, cos_angle},
		};
	}
	static Matrix3 rotateZ(T angle) {
		const T cos_angle = cos(angle);
		const T sin_angle = sin(angle);
		return {
			{cos_angle, sin_angle, 0},
			{-sin_angle, cos_angle, 0},
			{0, 0, 1},
		};
	}
};

using mat3 = Matrix3<f32>;

template <typename T = f32> struct Matrix4 {
	static_assert(std::is_floating_point_v<T>);

	using Vector = Vector4<T>;

	Vector columns[4];

	Matrix4() = default;
	Matrix4(Vector c0, Vector c1, Vector c2, Vector c3) : columns{c0, c1, c2, c3} {}

	Vector& operator[](int c) { return columns[c]; }
	const Vector& operator[](int c) const { return columns[c]; }

	constexpr static Matrix4 identity() {
		return {
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1},
		};
	}

	friend inline Vector operator*(const Matrix4& m, const Vector& v) {
		return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
	}

	friend inline Matrix4 operator*(const Matrix4& a, const Matrix4& b) {
		return {a * b[0], a * b[1], a * b[2], a * b[3]};
	}
	friend inline Matrix4 operator*=(Matrix4& a, const Matrix4& b) {
		return {a *= b[0], a *= b[1], a *= b[2], a *= b[3]};
	}

	template <typename R> friend R& operator>>(R& r, Matrix4& m) { return r >> m[0] >> m[1] >> m[2] >> m[3]; }

	static Matrix4 translate(Vector3<T> v) {
		Matrix4 m = identity();
		m[3].x = v.x;
		m[3].y = v.y;
		m[3].z = v.z;
		return m;
	}

	static Matrix4 lookAt(Vector3<T> eye, Vector3<T> center, Vector3<T> up) {
		Vector3<T> backwards = normalized(eye - center);
		Vector3<T> side = normalized(cross(up, backwards));
		up = normalized(cross(backwards, side));

		return {
			{side.x, up.x, backwards.x, 0},
			{side.y, up.y, backwards.y, 0},
			{side.z, up.z, backwards.z, 0},
			{-eye * side, -eye * up, -eye * backwards, 1},
		};
	}

	// This is a little special
	// It maps from OpenGL camera space to Vulkan NDC with reversed depth and infinite zFar
	static Matrix4 perspective(T fovy, T aspect, T zNear) {
		T halfFov = fovy / 2;
		T cot = cos(halfFov) / sin(halfFov);

		Matrix4 m;
		m[0][0] = -cot / aspect;
		m[1][1] = cot;
		m[2][3] = -1.0f;
		m[3][2] = zNear;

		return m;
	}
};

using mat4 = Matrix4<f32>;
