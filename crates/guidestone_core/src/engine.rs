use hw_import::HWImporter;

use crate::{
	camera::OrbitCamera, input, math::UVec2, model::ModelCache, FrameInfo, PlatformConfig, Renderer,
};
pub struct EngineInit {}

pub struct Engine {
	input: input::State,
	render: Box<dyn Renderer>,
	camera: OrbitCamera,
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
		let mut engine = Self {
			input: Default::default(),
			render,
			camera: OrbitCamera::new(6000.0),
		};
		engine.start_game();
		engine
	}

	pub fn start_game(&mut self) {
		let mut hw_import = HWImporter::load();

		let model_cache = ModelCache::load(
			&mut hw_import,
			vec!["r1/lightinterceptor/rl0/lod0/lightinterceptor.peo".to_string()],
		);

		self.render.set_model_cache(model_cache);
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

			self.camera.control(input);

			self.input.reset();
		}

		self.render.render_frame(FrameInfo {
			resize,
			camera: self.camera.get_camera(),
		});

		false
	}

	pub fn input(&mut self) -> &mut input::State {
		&mut self.input
	}
}
