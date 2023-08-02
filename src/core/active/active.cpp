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

			camera_system.main_camera.control(input_state);
			camera_system.main_camera.set_eye_position();
		}

		engine.render->renderFrame(Render::FrameInfo{camera_system.getActiveCamera()});
	}
}

Active::Active(Engine& e) : engine(e), input(e) { thread = std::thread(&Active::thread_func, this); }
void Active::start() { start_signal.release(); }
Active::~Active() { thread.join(); }
