use std::time::Duration;

use glam::{Mat3, Vec3};
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
		let mut this = Self {
			world: World::new(),
		};
		let mc = ModelCache::load(
			hw_import,
			vec![
				"r1/lightinterceptor/rl0/lod0/lightinterceptor.peo".into(),
				"r1/resourcecollector/rl0/lod0/resourcecollector.peo".into(),
				"r1/mothership/rl0/lod0/mothership.peo".into(),
			],
		);
		this.world.spawn((
			Common {
				model: 0,
				mass: 1.0,
				moment_of_inertia: (1.0, 1.0, 1.0).into(),
				max_velocity: 1.0,
				max_rot: 1.0,
			},
			PosInfo {
				position: (0.0, 0.0, 0.0).into(),
				velocity: (0.0, 0.0, 0.0).into(),
				force: (0.0, 0.0, 0.0).into(),
			},
			RotInfo {
				coord_sys: Mat3::IDENTITY,
				rot_speed: (0.0, 0.0, 0.0).into(),
				torque: (0.0, 1.0, 0.0).into(),
			},
		));
		this.world.spawn((
			Common {
				model: 1,
				mass: 1.0,
				moment_of_inertia: (1.0, 1.0, 1.0).into(),
				max_velocity: 1.0,
				max_rot: 1.0,
			},
			PosInfo {
				position: (10.0, 0.0, 0.0).into(),
				velocity: (0.0, 0.0, 0.0).into(),
				force: (0.0, 0.0, 0.0).into(),
			},
			RotInfo {
				coord_sys: Mat3::IDENTITY,
				rot_speed: (0.0, 0.0, 0.0).into(),
				torque: (1.0, 0.0, 0.0).into(),
			},
		));
		this.world.spawn((
			Common {
				model: 2,
				mass: 1.0,
				moment_of_inertia: (1.0, 1.0, 1.0).into(),
				max_velocity: 1.0,
				max_rot: 1.0,
			},
			PosInfo {
				position: (-10.0, 0.0, 0.0).into(),
				velocity: (0.0, 0.0, 0.0).into(),
				force: (0.0, 0.0, 0.0).into(),
			},
			RotInfo {
				coord_sys: Mat3::IDENTITY,
				rot_speed: (0.0, 0.0, 0.0).into(),
				torque: (0.0, 0.0, 1.0).into(),
			},
		));
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

			rot_info.coord_sys *= Mat3::from_rotation_x(rotate.x)
				* Mat3::from_rotation_y(rotate.y)
				* Mat3::from_rotation_z(rotate.z);
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
