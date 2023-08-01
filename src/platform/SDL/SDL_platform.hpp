#pragma once

#include "engine.hpp"
#include "platform.hpp"
#include <SDL.h>

class SDLPlatform final : public Platform {
	std::vector<std::string> args;

	std::unique_ptr<Engine> engine;

	SDL_Window* window;

	std::binary_semaphore shutdown_semaphore{0};

  public:
	SDLPlatform(SDLPlatform&) = delete;
	SDLPlatform(SDLPlatform&&) = delete;
	SDLPlatform(int argc, char* argv[]);

	static int event_proc(void* userdata, SDL_Event* event);
	void event_proc(const SDL_Event&);
	void run();
	~SDLPlatform();

	std::unique_ptr<Render> init_video() override;
	void set_relative_mouse(bool enable) override;
	void shutdown() override;
};
