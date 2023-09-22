use std::{
	array,
	iter::Sum,
	ops::{Add, Deref, DerefMut, Div, Index, IndexMut, Mul, Neg, Sub},
};

#[repr(C)]
#[derive(Clone, Copy, PartialEq)]
pub struct Vector<T, const N: usize>([T; N]);

// Unsized Vector Functions
// Loops in this section will (hopefully) be optimized by the compiler

impl<T, const N: usize> Default for Vector<T, N>
where
	[T; N]: Default,
{
	fn default() -> Self {
		Self(Default::default())
	}
}

impl<T, const N: usize> From<[T; N]> for Vector<T, N> {
	fn from(value: [T; N]) -> Self {
		Vector(value)
	}
}

impl<T, const N: usize, Idx> Index<Idx> for Vector<T, N>
where
	[T; N]: Index<Idx>,
{
	type Output = <[T; N] as Index<Idx>>::Output;

	fn index(&self, index: Idx) -> &Self::Output {
		self.0.index(index)
	}
}
impl<T, const N: usize, Idx> IndexMut<Idx> for Vector<T, N>
where
	[T; N]: IndexMut<Idx>,
{
	fn index_mut(&mut self, index: Idx) -> &mut Self::Output {
		self.0.index_mut(index)
	}
}
impl<T, const N: usize> IntoIterator for Vector<T, N> {
	type Item = T;

	type IntoIter = array::IntoIter<T, N>;

	fn into_iter(self) -> Self::IntoIter {
		self.0.into_iter()
	}
}

// Fails due to conflicting From<T> for T
// impl<T, const N: usize, R> From<Vector<R, N>> for Vector<T, N>
// where
// 	T: From<R>,
// {
// 	fn from(value: Vector<R, N>) -> Self {
// 		value.0.map(|r| T::from(r))
// 	}
// }

impl<T: Neg, const N: usize> Neg for Vector<T, N> {
	type Output = Vector<T::Output, N>;

	fn neg(self) -> Self::Output {
		Vector(self.0.map(Neg::neg))
	}
}

impl<T: Mul<M>, M: Copy, const N: usize> Mul<M> for Vector<T, N> {
	type Output = Vector<T::Output, N>;

	fn mul(self, rhs: M) -> Self::Output {
		Vector(self.0.map(|lhs| lhs * rhs))
	}
}

impl<T: Div<D>, D: Copy, const N: usize> Div<D> for Vector<T, N> {
	type Output = Vector<T::Output, N>;

	fn div(self, rhs: D) -> Self::Output {
		Vector(self.0.map(|lhs| lhs / rhs))
	}
}

impl<T: Copy + Add<Output = T>, const N: usize> Add<Vector<T, N>> for Vector<T, N> {
	type Output = Vector<T, N>;

	fn add(self, rhs: Vector<T, N>) -> Self::Output {
		Vector(array::from_fn(|i| self[i] + rhs[i]))
	}
}

impl<T: Copy + Add<Output = T>, const N: usize> Sum<Vector<T, N>> for Vector<T, N>
where
	Vector<T, N>: Default,
{
	fn sum<I: Iterator<Item = Vector<T, N>>>(iter: I) -> Self {
		iter.fold(Default::default(), |acc, v| acc + v)
	}
}

impl<T: Copy + Sub<R>, R: Copy, const N: usize> Sub<Vector<R, N>> for Vector<T, N> {
	type Output = Vector<T::Output, N>;

	fn sub(self, rhs: Vector<R, N>) -> Self::Output {
		Vector(array::from_fn(|i| self[i] - rhs[i]))
	}
}
impl<T: Mul, const N: usize> Vector<T, N> {
	pub fn dot<D: Sum<T::Output>>(self, lhs: Self) -> D {
		self.into_iter().zip(lhs).map(|(a, b)| a * b).sum()
	}
}

impl<T: Copy + Mul> Vector<T, 3> {
	pub fn cross(self, lhs: Self) -> Self
	where
		T::Output: Sub<Output = T>,
	{
		Vector([
			self.y * lhs.z - self.z * lhs.y,
			self.z * lhs.x - self.x * lhs.z,
			self.x * lhs.y - self.y * lhs.x,
		])
	}
}

impl<T: Copy + Mul, const N: usize> Vector<T, N> {
	pub fn length_squared<L: Sum<T::Output>>(self) -> L {
		self.into_iter().map(|v| v * v).sum()
	}
}

impl<T: Float, const N: usize> Vector<T, N> {
	pub fn length(self) -> T {
		self.length_squared::<T>().sqrt()
	}
	pub fn normalized(self) -> Self {
		let magnitude = self.length();
		if magnitude.is_normal() {
			self / magnitude
		} else {
			self
		}
	}
}

// Sized Vector Functions

impl<T> Vector<T, 2> {
	pub fn new(x: T, y: T) -> Self {
		Self([x, y])
	}
}
impl<T> Vector<T, 3> {
	pub fn new(x: T, y: T, z: T) -> Self {
		Self([x, y, z])
	}
}
impl<T> Vector<T, 4> {
	pub fn new(x: T, y: T, z: T, w: T) -> Self {
		Self([x, y, z, w])
	}
}

macro_rules! vector_deref {
	($n:expr, $target:ident) => {
		impl<T> Deref for Vector<T, $n> {
			type Target = deref::$target<T>;

			fn deref(&self) -> &Self::Target {
				unsafe { &*(self as *const Self).cast() }
			}
		}

		impl<T> DerefMut for Vector<T, $n> {
			fn deref_mut(&mut self) -> &mut Self::Target {
				unsafe { &mut *(self as *mut Self).cast() }
			}
		}
	};
}

vector_deref!(2, Xy);
vector_deref!(3, Xyz);
vector_deref!(4, Xyzw);

mod deref {

	#[repr(C)]
	pub struct Xy<T> {
		pub x: T,
		pub y: T,
	}

	#[repr(C)]
	pub struct Xyz<T> {
		pub x: T,
		pub y: T,
		pub z: T,
	}

	#[repr(C)]
	pub struct Xyzw<T> {
		pub x: T,
		pub y: T,
		pub z: T,
		pub w: T,
	}
}

mod alias {
	use super::Vector;

	pub type BoolVec2 = Vector<bool, 2>;
	pub type BoolVec3 = Vector<bool, 3>;
	pub type BoolVec4 = Vector<bool, 4>;

	pub type I8Vec2 = Vector<i8, 2>;
	pub type I8Vec3 = Vector<i8, 3>;
	pub type I8Vec4 = Vector<i8, 4>;
	pub type I16Vec2 = Vector<i16, 2>;
	pub type I16Vec3 = Vector<i16, 3>;
	pub type I16Vec4 = Vector<i16, 4>;
	pub type I32Vec2 = Vector<i32, 2>;
	pub type I32Vec3 = Vector<i32, 3>;
	pub type I32Vec4 = Vector<i32, 4>;
	pub type I64Vec2 = Vector<i64, 2>;
	pub type I64Vec3 = Vector<i64, 3>;
	pub type I64Vec4 = Vector<i64, 4>;

	pub type U8Vec2 = Vector<u8, 2>;
	pub type U8Vec3 = Vector<u8, 3>;
	pub type U8Vec4 = Vector<u8, 4>;
	pub type U16Vec2 = Vector<u16, 2>;
	pub type U16Vec3 = Vector<u16, 3>;
	pub type U16Vec4 = Vector<u16, 4>;
	pub type U32Vec2 = Vector<u32, 2>;
	pub type U32Vec3 = Vector<u32, 3>;
	pub type U32Vec4 = Vector<u32, 4>;
	pub type U64Vec2 = Vector<u64, 2>;
	pub type U64Vec3 = Vector<u64, 3>;
	pub type U64Vec4 = Vector<u64, 4>;

	pub type F32Vec2 = Vector<f32, 2>;
	pub type F32Vec3 = Vector<f32, 3>;
	pub type F32Vec4 = Vector<f32, 4>;
	pub type F64Vec2 = Vector<f64, 2>;
	pub type F64Vec3 = Vector<f64, 3>;
	pub type F64Vec4 = Vector<f64, 4>;

	pub type BVec2 = BoolVec2;
	pub type BVec3 = BoolVec3;
	pub type BVec4 = BoolVec4;

	pub type UVec2 = U32Vec2;
	pub type UVec3 = U32Vec3;
	pub type UVec4 = U32Vec4;
	pub type IVec2 = I32Vec2;
	pub type IVec3 = I32Vec3;
	pub type IVec4 = I32Vec4;

	pub type Vec2 = F32Vec2;
	pub type Vec3 = F32Vec3;
	pub type Vec4 = F32Vec4;

	pub type DVec2 = F64Vec2;
	pub type DVec3 = F64Vec3;
	pub type DVec4 = F64Vec4;
}

pub use alias::*;

use super::num_traits::Float;
