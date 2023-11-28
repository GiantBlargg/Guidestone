pub use camera::Camera;
pub use common::math;
pub use engine::Engine;
use math::{Mat3, UVec2, Vec3};
use model::ModelCache;

mod camera;
mod engine;
pub mod input;
pub mod model;
mod universe;

#[must_use]
pub struct PlatformConfig {
	pub min_res: UVec2,
	pub initial_res: UVec2,
	pub window_title: String,
}

pub struct FrameInfo {
	pub resize: Option<UVec2>,
	pub camera: Camera,
}

pub struct RenderItem {
	pub model: u32,
	pub position: Vec3,
	pub rotation: Mat3,
}

pub type RenderList = Vec<RenderItem>;

pub trait Renderer {
	fn render_frame(&mut self, frame: FrameInfo);

	fn set_model_cache(&mut self, model_cache: ModelCache);

	fn set_render_list(&mut self, render_list: RenderList);
}
