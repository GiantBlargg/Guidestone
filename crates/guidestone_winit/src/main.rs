use std::sync::Arc;

use env_logger::fmt::style::Style;
use guidestone_core::{
	input,
	math::{UVec2, Vec2},
	Engine,
};
use winit::{
	dpi::{PhysicalPosition, PhysicalSize},
	event::{DeviceEvent, Event, MouseButton, MouseScrollDelta, WindowEvent},
	event_loop::EventLoop,
	window::{CursorGrabMode, Window, WindowBuilder},
};

#[derive(Default)]
struct MouseGrabber {
	last_pos: PhysicalPosition<f64>,
	is_grabbed: bool,
	manual_lock: bool,
}

impl MouseGrabber {
	fn cursor_moved(&mut self, window: &Window, pos: PhysicalPosition<f64>) {
		if self.manual_lock {
			window.set_cursor_position(self.last_pos).unwrap();
		} else {
			self.last_pos = pos;
		}
	}
	fn grab(&mut self, window: &Window, grab: bool) {
		if grab {
			if window.set_cursor_grab(CursorGrabMode::Locked).is_err() {
				window.set_cursor_grab(CursorGrabMode::Confined).unwrap();
				self.manual_lock = true;
			}
		} else {
			self.manual_lock = false;
			window.set_cursor_grab(CursorGrabMode::None).unwrap();
		}
		self.is_grabbed = grab;
		window.set_cursor_visible(!grab);
	}
}

fn log_init() {
	use std::{io::Write, time::Instant};

	let start = Instant::now();
	env_logger::builder()
		.format(move |buf, record| {
			let time_since_start = Instant::now() - start;

			let time_style = Style::new().dimmed();
			let time_reset = time_style.render_reset();
			let time_style = time_style.render();
			let level_style = buf.default_level_style(record.level()).bold();
			let level_reset = level_style.render_reset();
			let level_style = level_style.render();

			writeln!(
				buf,
				"{time_style}{}.{:06}]{time_reset} {level_style}{:5} {}:{level_reset} {}",
				time_since_start.as_secs(),
				time_since_start.subsec_micros(),
				record.level(),
				record.target(),
				record.args()
			)
		})
		.init();
}

fn main() {
	log_init();

	if std::env::var("FORCE_X11").is_ok() {
		std::env::remove_var("WAYLAND_DISPLAY");
	}

	let event_loop = EventLoop::new().unwrap();

	let (platform_config, engine_init) = Engine::init();

	let convert_size = |s: UVec2| PhysicalSize {
		width: s.x,
		height: s.y,
	};
	let window = Arc::new(
		WindowBuilder::new()
			.with_inner_size(convert_size(platform_config.initial_res))
			.with_min_inner_size(convert_size(platform_config.min_res))
			.with_title(platform_config.window_title)
			.build(&event_loop)
			.unwrap(),
	);
	let mut mouse_grabber = MouseGrabber::default();

	let render = Box::new(futures_lite::future::block_on(render_wgpu::Render::new(
		window.clone(),
	)));

	let mut engine = Engine::start(engine_init, render);

	event_loop
		.run(|event, elwt| {
			let mut send_event: Option<input::Event> = None;

			match event {
				Event::WindowEvent {
					event: window_event,
					..
				} => {
					send_event = match window_event {
						WindowEvent::CloseRequested => Some(input::Event::Quit),
						WindowEvent::Resized(size) => {
							Some(input::Event::Resize(UVec2::new(size.width, size.height)))
						}
						WindowEvent::CursorMoved { position: pos, .. } => {
							mouse_grabber.cursor_moved(&window, pos);
							if mouse_grabber.is_grabbed {
								None
							} else {
								Some(input::Event::MousePosition(Some(Vec2::new(
									pos.x as f32,
									pos.y as f32,
								))))
							}
						}
						WindowEvent::CursorLeft { device_id: _ } => {
							Some(input::Event::MousePosition(None))
						}
						WindowEvent::MouseInput { state, button, .. } => match button {
							MouseButton::Left => Some(input::Button::MouseLeft),
							MouseButton::Right => Some(input::Button::MouseRight),
							MouseButton::Middle => Some(input::Button::MouseMiddle),
							_ => None,
						}
						.map(|button| input::Event::Button {
							button,
							pressed: state.is_pressed(),
						}),
						WindowEvent::MouseWheel { delta, .. } => {
							Some(input::Event::MouseScroll(match delta {
								MouseScrollDelta::LineDelta(_, y) => y,
								MouseScrollDelta::PixelDelta(PhysicalPosition { x: _, y }) => {
									(y * 16.0) as f32
								}
							}))
						}
						_ => None,
					}
				}
				Event::DeviceEvent {
					device_id: _,
					event: DeviceEvent::MouseMotion { delta: (x, y) },
				} => {
					if mouse_grabber.is_grabbed {
						send_event =
							Some(input::Event::MouseRelative(Vec2::new(x as f32, y as f32)))
					}
				}
				Event::AboutToWait => {
					if engine.update() {
						elwt.exit();
					}
				}
				_ => (),
			}

			let response = send_event.and_then(|e| engine.input().event(e));

			if let Some(r) = response {
				match r {
					input::Response::GrabMouse(grab) => mouse_grabber.grab(&window, grab),
				};
			}
		})
		.unwrap();
}
