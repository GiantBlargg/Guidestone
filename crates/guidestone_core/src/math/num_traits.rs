use std::{
	iter::Sum,
	ops::{Add, Div, Mul, Neg, Sub},
};

pub trait Float:
	Sized
	+ Copy
	+ Default
	+ From<i8>
	+ From<f32>
	+ PartialEq
	+ Neg<Output = Self>
	+ Add<Output = Self>
	+ Sum<Self>
	+ Sub<Output = Self>
	+ Mul<Output = Self>
	+ Div<Output = Self>
{
	fn sqrt(self) -> Self;
	fn sin(self) -> Self;
	fn cos(self) -> Self;
	fn tan(self) -> Self;

	fn is_nan(self) -> bool;
	fn is_infinite(self) -> bool;
	fn is_finite(self) -> bool;
	fn is_subnormal(self) -> bool;
	fn is_normal(self) -> bool;
}

macro_rules! forward {
	{$(fn $f:ident(self) -> $r:ty;)+} => {
		$(
		#[inline]
		fn $f(self) -> $r {
			self.$f()
		}
	)+
	};
}

macro_rules! impl_float {
	($t:ty) => {
		impl Float for $t {
			forward! {
			fn sqrt(self) -> Self;
			fn sin(self) -> Self;
			fn cos(self) -> Self;
			fn tan(self) -> Self;

			fn is_nan(self) -> bool;
			fn is_infinite(self) -> bool;
			fn is_finite(self) -> bool;
			fn is_subnormal(self) -> bool;
			fn is_normal(self) -> bool;
			}
		}
	};
}

impl_float!(f32);
impl_float!(f64);
