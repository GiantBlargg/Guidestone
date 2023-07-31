#include "engine.hpp"

#include "model.hpp"

Engine::Engine(Platform& p) : platform(p) {}

void Engine::init() {
	render = platform.init_video();

	interactive_thread = std::thread(&Engine::interactive_thread_func, this);
};

Engine::~Engine() { interactive_thread.join(); }

void Engine::startGame() {
	ModelCache model_cache;
	model_cache.loadClassicModel("r1/lightcorvette/rl0/lod0/lightcorvette.peo");
	render->setModelCache(model_cache);
}

template <class... Ts> struct overload : Ts... {
	using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;

void Engine::interactive_thread_func() {
	// Not a true init step, just here for testing
	// TODO: Move somewhere more suitable
	startGame();

	while (true) {
		bool shutdown = false;
		const std::vector<Input::Event>& events = input.get_events();
		for (auto& event : events) {
			std::visit(
				overload{
					[this](const Event::Resize& resize) { render->resize(resize.width, resize.height); },
					[&shutdown](const Event::Quit&) { shutdown = true; }},
				event);
		}
		if (shutdown)
			break;

		render->renderFrame(Render::FrameInfo{camera_system.getActiveCamera()});
	}

	platform.shutdown();
}
