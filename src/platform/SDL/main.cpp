#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <chrono>
#include <iostream>

#include "engine/engine.hpp"
#include "render/vulkan/render.hpp"

int main(int argc, char* argv[]) {
#ifdef __LINUX__
	SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
#endif

	Engine engine;

	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow(
		"Guidestone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0,
		SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_SetWindowMinimumSize(window, 640, 480);

	Vulkan::Context::Create vulkan_context{
		(PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr(),
		{},
		[window](VkInstance instance, VkSurfaceKHR* surface) { SDL_Vulkan_CreateSurface(window, instance, surface); }};

	unsigned int sdl_vulkan_extensions_count;
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_vulkan_extensions_count, nullptr);
	vulkan_context.extensions.resize(sdl_vulkan_extensions_count);
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_vulkan_extensions_count, vulkan_context.extensions.data());

	Vulkan::Render* render = new Vulkan::Render(vulkan_context);

	engine.init(render);

	engine.startGame();

	SDL_ShowWindow(window);

	bool shutdown = false;

	auto last = std::chrono::steady_clock::now();
	double accum_time = 0;

	while (!shutdown) {
		auto now = std::chrono::steady_clock::now();
		double delta = std::chrono::duration<double>(now - last).count();
		last = now;
		accum_time += delta;

		while (accum_time > engine.tick_interval) {
			accum_time -= engine.tick_interval;
			engine.tickUpdate();
		}

		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				shutdown = true;
				break;
			case SDL_WINDOWEVENT: {
				switch (ev.window.event) {
				case SDL_WINDOWEVENT_SIZE_CHANGED: {
					int w, h;
					SDL_Vulkan_GetDrawableSize(window, &w, &h);
					render->resize(w, h);
				} break;
				}
			} break;
			}
		}

		engine.update(delta);
	}

	delete render;

	SDL_DestroyWindow(window);

	return 0;
}
