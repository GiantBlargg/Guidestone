use std::time::Duration;

use common::math::{Mat3, Vec3};
use hecs::World;
use hw_import::HWImporter;

use crate::{model::ModelCache, RenderItem, RenderList};

pub struct Common {
	pub model: u32,
	pub mass: f32,
	pub moment_of_inertia: Vec3,
	pub max_velocity: f32,
	pub max_rot: f32,
}
struct PosInfo {
	position: Vec3,
	velocity: Vec3,
	force: Vec3,
}
struct RotInfo {
	coord_sys: Mat3,
	rot_speed: Vec3,
	torque: Vec3,
}

pub struct Universe {
	world: World,
}

fn cap_vector(vec: Vec3, cap: f32) -> Vec3 {
	Vec3::new(
		vec.x.clamp(-cap, cap),
		vec.y.clamp(-cap, cap),
		vec.z.clamp(-cap, cap),
	)
}

impl Universe {
	pub fn new(hw_import: &mut HWImporter) -> (Self, ModelCache) {
		let this = Self {
			world: World::new(),
		};
		let mc = ModelCache::load(hw_import, vec![]);
		(this, mc)
	}
	pub fn update(&mut self, delta: Duration) {
		let delta_secs = delta.as_secs_f32();

		for (_, (common, pos_info)) in self.world.query_mut::<(&Common, &mut PosInfo)>() {
			let velocity = pos_info.velocity + (pos_info.force / common.mass) * delta_secs;

			pos_info.velocity = cap_vector(velocity, common.max_velocity);

			pos_info.position += pos_info.velocity * delta_secs;
		}

		for (_, (common, rot_info)) in self.world.query_mut::<(&Common, &mut RotInfo)>() {
			let rot_speed =
				rot_info.rot_speed + (rot_info.torque / common.moment_of_inertia) * delta_secs;

			rot_info.rot_speed = cap_vector(rot_speed, common.max_rot);

			let rotate = rot_info.rot_speed * delta_secs;

			rot_info.coord_sys *=
				Mat3::rotate_x(rotate.x) * Mat3::rotate_y(rotate.y) * Mat3::rotate_z(rotate.z);
		}
	}

	pub fn get_render_list(&self) -> RenderList {
		self.world
			.query::<(&Common, &PosInfo, &RotInfo)>()
			.iter()
			.map(|(_, (common, pos_info, rot_info))| RenderItem {
				model: common.model,
				position: pos_info.position,
				rotation: rot_info.coord_sys,
			})
			.collect()
	}
}
