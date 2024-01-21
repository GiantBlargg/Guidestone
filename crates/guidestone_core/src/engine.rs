use std::time::{Duration, Instant};

use hw_import::HWImporter;

use crate::{
	camera::OrbitCamera, input, math::UVec2, universe::Universe, FrameInfo, PlatformConfig,
	RenderList, Renderer,
};
pub struct EngineInit {}

pub struct Engine {
	input: input::State,
	render: Box<dyn Renderer>,
	camera: OrbitCamera,
	universe: Option<UniverseHolder>,
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
			universe: None,
		};
		engine.start_game();
		engine
	}

	pub fn start_game(&mut self) {
		let mut hw_import = HWImporter::load();

		let (universe, model_cache) = Universe::new(&mut hw_import);

		self.render.set_model_cache(model_cache);
		self.render.set_render_list(universe.get_render_list());

		self.universe = Some(UniverseHolder {
			universe,
			last_tick: Instant::now(),
		});
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

		if let Some(universe) = &mut self.universe {
			if universe.tick(now) {
				self.render.set_render_list(universe.get_render_list())
			}
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

struct UniverseHolder {
	universe: Universe,
	last_tick: Instant,
}

impl UniverseHolder {
	const UNIVERSE_RATE: u64 = 16;
	const UNIVERSE_PERIOD: Duration = Duration::from_nanos(1000000000 / Self::UNIVERSE_RATE);

	fn tick(&mut self, now: Instant) -> bool {
		let update_count = {
			let delta = now.duration_since(self.last_tick);
			let count = u32::try_from(delta.as_nanos() / Self::UNIVERSE_PERIOD.as_nanos())
				.unwrap_or(u32::MAX);
			self.last_tick += Self::UNIVERSE_PERIOD * count;
			count
		};

		if update_count == 0 {
			return false;
		}

		for _ in 0..update_count {
			self.universe.update(Self::UNIVERSE_PERIOD);
		}

		true
	}

	fn get_render_list(&self) -> RenderList {
		self.universe.get_render_list()
	}
}
