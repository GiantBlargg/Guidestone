#pragma once

#include "math.hpp"
#include "types.hpp"
#include <array>
#include <mutex>
#include <optional>

class Input {
  public:
	struct State {
		std::optional<uvec2> resize;
		bool quit_request = false;
	};

  private:
	std::mutex state_mutex;
	std::array<State, 2> state;
	size_t active_state = 0;

  public:
	const State& get_state() {
		std::unique_lock lock(state_mutex);
		const State& ret = state[active_state];
		active_state = (active_state + 1) % state.size();
		state[active_state] = State();
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
};
