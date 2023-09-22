use math::{UVec2, Vec3};

pub mod input;
pub mod math;

#[must_use]
pub struct PlatformConfig {
	pub min_res: UVec2,
	pub initial_res: UVec2,
	pub window_title: String,
}
pub struct EngineInit {}

#[derive(Default)]
pub struct Camera {
	pub eye: Vec3,
	pub target: Vec3,
	pub up: Vec3,
	pub fov: f32,
	pub near: f32,
}

pub struct FrameInfo {
	pub resize: Option<UVec2>,
}
pub trait Renderer {
	fn render_frame(&mut self, frame: FrameInfo);
}

pub struct Engine {
	input: input::State,
	render: Box<dyn Renderer>,
}

impl Engine {
	pub fn init() -> (PlatformConfig, EngineInit) {
		// TODO: Load from config
		let min_res: UVec2 = UVec2::new(640, 480);
		(
			PlatformConfig {
				min_res,
				initial_res: min_res,
				window_title: "Guidestone".to_string(),
			},
			EngineInit {},
		)
	}

	pub fn start(_: EngineInit, render: Box<dyn Renderer>) -> Self {
		Self {
			input: Default::default(),
			render,
		}
	}

	// Returns true when platform should shutdown.
	pub fn update(&mut self) -> bool {
		let resize;
		{
			let input = &self.input;

			if input.quit {
				return true;
			}

			resize = input.resize;

			self.input = Default::default();
		}

		self.render.render_frame(FrameInfo { resize });

		false
	}

	pub fn input(&mut self) -> &mut input::State {
		&mut self.input
	}
}
