#include "input.hpp"
#include "engine.hpp"

void Input::mouse_button(MouseButton button, bool pressed) {
	if (button == MouseButton::Right) {
		engine.platform.set_relative_mouse(pressed);
	}

	// TODO: Actually log the event
}
