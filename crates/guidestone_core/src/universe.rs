use std::time::Duration;

use glam::{Mat3, Vec3};
use hecs::World;

use hw_import::HWImporter;

use crate::{model::ModelCache, RenderItem, RenderList};

struct Model(u32);

struct PosInfo {
	mass: f32,
	max_velocity: f32,

	position: Vec3,
	velocity: Vec3,
	force: Vec3,
}
struct RotInfo {
	moment_of_inertia: Vec3,
	max_rot: f32,

	coord_sys: Mat3,
	rot_speed: Vec3,
	torque: Vec3,
}

pub struct Universe {
	world: World,
}

impl Universe {
	pub fn new(hw_import: &mut HWImporter) -> (Self, ModelCache) {
		let mut this = Self {
			world: World::new(),
		};
		let mc = ModelCache::load(
			hw_import,
			vec!["r1/lightcorvette/rl0/lod0/lightcorvette.peo".into()],
		);
		this.world.spawn((
			Model(0),
			PosInfo {
				mass: 1.0,
				max_velocity: 1.0,
				position: (0.0, 0.0, 0.0).into(),
				velocity: (0.0, 0.0, 0.0).into(),
				force: (0.0, 0.0, 0.0).into(),
			},
			RotInfo {
				moment_of_inertia: (1.0, 1.0, 1.0).into(),
				max_rot: 1.0,
				coord_sys: Mat3::IDENTITY,
				rot_speed: (0.0, 0.0, 0.0).into(),
				torque: (0.0, 0.0, 0.0).into(),
			},
		));
		(this, mc)
	}
	pub fn update(&mut self, delta: Duration) {
		let delta_secs = delta.as_secs_f32();

		for (_, pos_info) in self.world.query_mut::<&mut PosInfo>() {
			let velocity = pos_info.velocity + (pos_info.force / pos_info.mass) * delta_secs;

			pos_info.velocity = velocity.clamp_length_max(pos_info.max_velocity);

			pos_info.position += pos_info.velocity * delta_secs;

			pos_info.force = Vec3::ZERO;
		}

		for (_, rot_info) in self.world.query_mut::<&mut RotInfo>() {
			let rot_speed =
				rot_info.rot_speed + (rot_info.torque / rot_info.moment_of_inertia) * delta_secs;

			rot_info.rot_speed = rot_speed.clamp_length_max(rot_info.max_rot);

			let rotate = rot_info.rot_speed * delta_secs;

			rot_info.coord_sys *= Mat3::from_rotation_x(rotate.x)
				* Mat3::from_rotation_y(rotate.y)
				* Mat3::from_rotation_z(rotate.z);

			rot_info.torque = Vec3::ZERO;
		}
	}

	pub fn get_render_list(&self) -> RenderList {
		self.world
			.query::<(&Model, &PosInfo, &RotInfo)>()
			.iter()
			.map(|(_, (Model(model), pos_info, rot_info))| RenderItem {
				model: *model,
				position: pos_info.position,
				rotation: rot_info.coord_sys,
			})
			.collect()
	}
}
