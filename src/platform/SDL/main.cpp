#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "render/render.hpp"

int main(int argc, char* argv[]) {
#ifdef __LINUX__
	SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
#endif

	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow(
		"Guidestone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0,
		SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_SetWindowMinimumSize(window, 640, 480);

	std::vector<const char*> vulkan_extensions;
	uint sdl_vulkan_extensions_count;
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_vulkan_extensions_count, nullptr);
	vulkan_extensions.resize(sdl_vulkan_extensions_count);
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_vulkan_extensions_count, vulkan_extensions.data());

	render = new Render::Render(
		(PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr(), vulkan_extensions, [window](vk::Instance i) {
			VkSurfaceKHR surface;
			SDL_Vulkan_CreateSurface(window, i, &surface);
			return surface;
		});

	SDL_ShowWindow(window);

	bool shutdown = false;
	while (!shutdown) {
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
		render->renderFrame();
	}

	delete render;

	SDL_DestroyWindow(window);
}