#pragma once

#include "math.hpp"
#include "types.hpp"
#include <array>
#include <mutex>
#include <optional>

class Engine;

class Input {
	Engine& engine;

  public:
	struct State {
		bool quit_request = false;
		std::optional<uvec2> resize;
		std::optional<vec2> mouse_position;
		vec2 mouse_relative = {0, 0};
	};

  private:
	std::mutex state_mutex;
	std::array<State, 2> state;
	size_t active_state = 0;

  public:
	Input(Engine& e) : engine(e) {}

	const State& get_state() {
		std::unique_lock lock(state_mutex);
		const State& ret = state[active_state];
		active_state = (active_state + 1) % state.size();
		state[active_state] = State();
		state[active_state].mouse_position = ret.mouse_position;
		return ret;
	}

	void quit_request() {
		std::unique_lock lock(state_mutex);
		state[active_state].quit_request = true;
	}
	void resize(uvec2 size) {
		std::unique_lock lock(state_mutex);
		state[active_state].resize = size;
	}

	enum class MouseButton { Left, Right, Middle };
	void mouse_button(MouseButton button, bool pressed);

	void mouse_position(std::optional<vec2> pos) {
		std::unique_lock lock(state_mutex);
		state[active_state].mouse_position = pos;
	}
	void mouse_relative(vec2 rel) {
		std::unique_lock lock(state_mutex);
		state[active_state].mouse_relative += rel;
	}
};
