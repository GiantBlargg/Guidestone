pub use camera::Camera;
pub use engine::Engine;
use math::UVec2;
use model::ModelCache;

mod camera;
mod engine;
mod fs;
pub mod input;
pub mod math;
pub mod model;

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
pub trait Renderer {
	fn render_frame(&mut self, frame: FrameInfo);

	fn set_model_cache(&mut self, model_cache: ModelCache);
}
