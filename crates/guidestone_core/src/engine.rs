use std::time::{Duration, Instant};

use hw_import::HWImporter;

use crate::{
	camera::OrbitCamera, input, math::UVec2, model::ModelCache, universe::Universe, FrameInfo,
	PlatformConfig, Renderer,
};
pub struct EngineInit {}

pub struct Engine {
	input: input::State,
	render: Box<dyn Renderer>,
	camera: OrbitCamera,
	last_tick: Option<Instant>,
	universe: Universe,
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
			universe: Default::default(),
			last_tick: None,
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

		self.last_tick = Some(Instant::now())
	}

	// Returns true when platform should shutdown.
	pub fn update(&mut self) -> bool {
		let now = Instant::now();

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

		self.tick(now);

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

impl Engine {
	const UNIVERSE_RATE: u64 = 16;
	const UNIVERSE_PERIOD: Duration = Duration::from_nanos(1000000000 / Self::UNIVERSE_RATE);

	fn tick(&mut self, now: Instant) {
		let update_count = if let Some(last) = &mut self.last_tick {
			let delta = now.duration_since(*last);
			let count = u32::try_from(delta.as_nanos() / Self::UNIVERSE_PERIOD.as_nanos())
				.unwrap_or(u32::MAX);
			*last += Self::UNIVERSE_PERIOD * count;
			count
		} else {
			0
		};

		if update_count == 0 {
			return;
		}

		for _ in 0..update_count {
			self.universe.update(Self::UNIVERSE_PERIOD);
		}

		self.render.set_render_list(self.universe.get_render_list())
	}
}
