use std::{
	array,
	iter::Sum,
	ops::{Index, IndexMut, Mul},
};

use super::{num_traits::Float, Vector};

#[repr(C)]
#[derive(Clone, Copy, PartialEq)]
pub struct Matrix<T, const N: usize, const M: usize>([Vector<T, M>; N]);

impl<T, const N: usize, const M: usize> Default for Matrix<T, N, M>
where
	[Vector<T, M>; N]: Default,
{
	fn default() -> Self {
		Self(Default::default())
	}
}

impl<T, const N: usize, const M: usize> From<[[T; M]; N]> for Matrix<T, N, M> {
	fn from(value: [[T; M]; N]) -> Self {
		Self(value.map(From::from))
	}
}

impl<T: From<i8>, const N: usize> Matrix<T, N, N> {
	pub fn identity() -> Self {
		Self(array::from_fn(|i| {
			array::from_fn(|j| if i == j { 1 } else { 0 }.into()).into()
		}))
	}
}

impl<T, const N: usize, const M: usize, Idx> Index<Idx> for Matrix<T, N, M>
where
	[Vector<T, M>; N]: Index<Idx>,
{
	type Output = <[Vector<T, M>; N] as Index<Idx>>::Output;

	fn index(&self, index: Idx) -> &Self::Output {
		self.0.index(index)
	}
}
impl<T, const N: usize, const M: usize, Idx> IndexMut<Idx> for Matrix<T, N, M>
where
	[Vector<T, M>; N]: IndexMut<Idx>,
{
	fn index_mut(&mut self, index: Idx) -> &mut Self::Output {
		self.0.index_mut(index)
	}
}

impl<T: Copy + Mul, const N: usize, const M: usize> Mul<Vector<T, N>> for Matrix<T, N, M>
where
	Vector<T::Output, M>: Sum,
{
	type Output = Vector<T::Output, M>;

	fn mul(self, rhs: Vector<T, N>) -> Self::Output {
		self.0.into_iter().zip(rhs).map(|(m, v)| m * v).sum()
	}
}

impl<T: Copy, const N: usize, const M: usize, const P: usize> Mul<Matrix<T, P, N>>
	for Matrix<T, N, M>
where
	Matrix<T, N, M>: Mul<Vector<T, N>, Output = Vector<T, M>>,
{
	type Output = Matrix<T, P, M>;

	fn mul(self, rhs: Matrix<T, P, N>) -> Self::Output {
		Matrix(array::from_fn(|i| self * rhs[i]))
	}
}

impl<T: Float> Matrix<T, 4, 4> {
	pub fn look_at(eye: Vector<T, 3>, center: Vector<T, 3>, up: Vector<T, 3>) -> Self {
		let backwards = (eye - center).normalized();
		let side = up.cross(backwards).normalized();
		let new_up = backwards.cross(side).normalized();

		[
			[side.x, new_up.x, backwards.x, 0.into()],
			[side.y, new_up.y, backwards.y, 0.into()],
			[side.z, new_up.z, backwards.z, 0.into()],
			[
				(-eye).dot(side),
				(-eye).dot(new_up),
				(-eye).dot(backwards),
				1.into(),
			],
		]
		.into()
	}

	pub fn perspective(fov_y: T, aspect: T, z_near: T) -> Self {
		let half_fov = fov_y / 2.into();
		let cot = half_fov.cos() / half_fov.sin();

		let mut m = Self::default();
		m[0][0] = -cot / aspect;
		m[1][1] = cot;
		m[2][3] = (-1).into();
		m[3][2] = z_near;

		m
	}
}

mod alias {
	use super::Matrix;

	pub type F32Matrix2x2 = Matrix<f32, 2, 2>;
	pub type F32Matrix2x3 = Matrix<f32, 2, 3>;
	pub type F32Matrix2x4 = Matrix<f32, 2, 4>;
	pub type F32Matrix3x2 = Matrix<f32, 3, 2>;
	pub type F32Matrix3x3 = Matrix<f32, 3, 3>;
	pub type F32Matrix3x4 = Matrix<f32, 3, 4>;
	pub type F32Matrix4x2 = Matrix<f32, 4, 2>;
	pub type F32Matrix4x3 = Matrix<f32, 4, 3>;
	pub type F32Matrix4x4 = Matrix<f32, 4, 4>;

	pub type F64Matrix2x2 = Matrix<f64, 2, 2>;
	pub type F64Matrix2x3 = Matrix<f64, 2, 3>;
	pub type F64Matrix2x4 = Matrix<f64, 2, 4>;
	pub type F64Matrix3x2 = Matrix<f64, 3, 2>;
	pub type F64Matrix3x3 = Matrix<f64, 3, 3>;
	pub type F64Matrix3x4 = Matrix<f64, 3, 4>;
	pub type F64Matrix4x2 = Matrix<f64, 4, 2>;
	pub type F64Matrix4x3 = Matrix<f64, 4, 3>;
	pub type F64Matrix4x4 = Matrix<f64, 4, 4>;

	pub type Mat2 = F32Matrix2x2;
	pub type Mat2x3 = F32Matrix2x3;
	pub type Mat2x4 = F32Matrix2x4;
	pub type Mat3x2 = F32Matrix3x2;
	pub type Mat3 = F32Matrix3x3;
	pub type Mat3x4 = F32Matrix3x4;
	pub type Mat4x2 = F32Matrix4x2;
	pub type Mat4x3 = F32Matrix4x3;
	pub type Mat4 = F32Matrix4x4;

	pub type DMat2 = F64Matrix2x2;
	pub type DMat2x3 = F64Matrix2x3;
	pub type DMat2x4 = F64Matrix2x4;
	pub type DMat3x2 = F64Matrix3x2;
	pub type DMat3 = F64Matrix3x3;
	pub type DMat3x4 = F64Matrix3x4;
	pub type DMat4x2 = F64Matrix4x2;
	pub type DMat4x3 = F64Matrix4x3;
	pub type DMat4 = F64Matrix4x4;
}
pub use alias::*;
