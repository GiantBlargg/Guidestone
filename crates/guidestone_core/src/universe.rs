use std::time::Duration;

use common::math::{Mat3, Vec3};
use hecs::{Entity, World};

use crate::RenderItem;

struct StaticCommon {
	model: u32,
	mass: f32,
	moment_of_inertia: Vec3,
}

struct StaticInfo(Entity);
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

#[derive(Default)]
pub struct Universe {
	world: World,
}

impl Universe {
	pub fn update(&mut self, delta: Duration) {
		let delta_secs = delta.as_secs_f32();
		let mut static_infos = self.world.query::<&StaticCommon>();
		let static_infos = static_infos.view();

		for (_, (static_info, pos_info)) in self.world.query::<(&StaticInfo, &mut PosInfo)>().iter()
		{
			let static_common = static_infos.get(static_info.0).unwrap();

			pos_info.velocity += (pos_info.force / static_common.mass) * delta_secs;

			// TODO: Cap Velocity

			pos_info.position += pos_info.velocity * delta_secs;
		}

		for (_, (static_info, rot_info)) in self.world.query::<(&StaticInfo, &mut RotInfo)>().iter()
		{
			let static_common = static_infos.get(static_info.0).unwrap();

			rot_info.rot_speed += (rot_info.torque / static_common.moment_of_inertia) * delta_secs;

			// TODO: Cap Velocity

			let rotate = rot_info.rot_speed * delta_secs;

			rot_info.coord_sys *=
				Mat3::rotate_x(rotate.x) * Mat3::rotate_y(rotate.y) * Mat3::rotate_z(rotate.z);
		}
	}

	pub fn get_render_list(&mut self) -> Vec<RenderItem> {
		let mut static_infos = self.world.query::<&StaticCommon>();
		let static_infos = static_infos.view();

		self.world
			.query::<(&StaticInfo, &PosInfo, &RotInfo)>()
			.iter()
			.map(|(_, (static_info, pos_info, rot_info))| {
				let static_common = static_infos.get(static_info.0).unwrap();
				RenderItem {
					model: static_common.model,
					position: pos_info.position,
					rotation: rot_info.coord_sys,
				}
			})
			.collect()
	}
}
