use crate::math::{UVec2, Vec2};

#[derive(Default)]
pub struct State {
	pub(crate) quit: bool,
	pub(crate) resize: Option<UVec2>,
	pub(crate) mouse_position: Option<Vec2>,
	pub(crate) mouse_relative: Vec2,
	pub(crate) scroll: f32,
}

pub enum Button {
	MouseLeft,
	MouseRight,
	MouseMiddle,
}
pub enum Event {
	Quit,
	Resize(UVec2),
	Button { button: Button, pressed: bool },
	MousePosition(Option<Vec2>),
	MouseRelative(Vec2),
	MouseScroll(f32),
}

#[must_use]
#[derive(Clone, Copy)]
pub enum Response {
	GrabMouse(bool),
}

impl State {
	pub fn event(&mut self, event: Event) -> Option<Response> {
		match event {
			Event::Quit => self.quit = true,
			Event::Resize(size) => self.resize = size.into(),
			Event::Button {
				button: Button::MouseRight,
				pressed,
			} => return Some(Response::GrabMouse(pressed)),
			Event::Button {
				button: _,
				pressed: _,
			} => (),
			Event::MousePosition(pos) => self.mouse_position = pos,
			Event::MouseRelative(rel) => self.mouse_relative = rel,
			Event::MouseScroll(scroll) => self.scroll = scroll,
		};
		None
	}

	pub fn reset(&mut self) {
		self.resize = Default::default();
		self.mouse_relative = Default::default();
		self.scroll = Default::default();
	}
}
