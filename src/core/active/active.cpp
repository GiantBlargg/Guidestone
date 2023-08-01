#include "active.hpp"
#include "engine.hpp"

void Active::thread_func() {
	start_signal.acquire();

	// Not a true init step, just here for testing
	// TODO: Move somewhere more suitable
	engine.startGame();

	while (true) {
		{
			const Input::State& input_state = input.get_state();

			if (input_state.quit_request) {
				engine.platform.shutdown();
				return;
			}
			if (input_state.resize.has_value()) {
				engine.render->resize(input_state.resize.value());
			}
		}

		engine.render->renderFrame(Render::FrameInfo{camera_system.getActiveCamera()});
	}
}

Active::Active(Engine& e) : engine(e) { thread = std::thread(&Active::thread_func, this); }
void Active::start() { start_signal.release(); }
Active::~Active() { thread.join(); }
