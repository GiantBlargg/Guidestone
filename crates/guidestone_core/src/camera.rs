use std::f32::consts::TAU;

use crate::{
	input,
	math::{Mat3, Vec3},
};

#[derive(Clone)]
pub struct Camera {
	pub eye: Vec3,
	pub target: Vec3,
	pub up: Vec3,
	pub fov: f32,
	pub near: f32,
}

impl Default for Camera {
	fn default() -> Self {
		Self {
			eye: Default::default(),
			target: Default::default(),
			up: Vec3::new(0.0, 0.0, 1.0),
			fov: 75_f32.to_radians(),
			near: 15.0,
		}
	}
}

pub struct OrbitCamera {
	angle: f32,
	declination: f32,
	distance: f32,
	target: Vec3,
	min_distance: f32,
	max_distnace: f32,
}

impl OrbitCamera {
	pub fn new(distance: f32) -> Self {
		Self {
			angle: 0.0,
			declination: 0.0,
			distance,
			target: Default::default(),
			min_distance: 50.0,
			max_distnace: 14500.0,
		}
	}

	pub fn control(&mut self, input: &input::State) {
		let zoom = 1.0 - 0.1 * input.scroll;
		self.distance = (self.distance * zoom).clamp(self.min_distance, self.max_distnace);
		let rot_sens = 0.1_f32.to_radians();
		self.angle = (self.angle + rot_sens * input.mouse_relative.x).rem_euclid(TAU);
		let max_declination = 85_f32.to_radians();
		self.declination = (self.declination + rot_sens * input.mouse_relative.y)
			.clamp(-max_declination, max_declination);
	}

	pub fn get_camera(&mut self) -> Camera {
		let eye = self.target
			+ (Mat3::rotate_z(self.angle)
				* Mat3::rotate_y(self.declination)
				* Vec3::new(self.distance, 0.0, 0.0));
		Camera {
			eye,
			target: self.target,
			..Default::default()
		}
	}
}
