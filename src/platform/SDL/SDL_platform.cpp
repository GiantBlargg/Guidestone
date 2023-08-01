#include "SDL_platform.hpp"

#include "log.hpp"
#include "vulkan_render.hpp"
#include <SDL_vulkan.h>

SDLPlatform::SDLPlatform(int argc, char* argv[]) : args(argv, argv + argc) {
	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
}

void SDLPlatform::run() {
	engine = std::make_unique<Engine>(*this);

	SDL_SetEventFilter(SDLPlatform::event_proc, this);

	engine->init();

	while (!shutdown_semaphore.try_acquire()) {
		SDL_PumpEvents();
	}

	SDL_SetEventFilter(nullptr, nullptr);

	engine.reset();

	if (window) {
		SDL_DestroyWindow(window);
	}
}

SDLPlatform::~SDLPlatform() { SDL_Quit(); }

int SDLPlatform::event_proc(void* userdata, SDL_Event* event) {
	static_cast<SDLPlatform*>(userdata)->event_proc(*event);
	return 0;
}
void SDLPlatform::event_proc(const SDL_Event& ev) {
	switch (ev.type) {
	case SDL_QUIT: {
		engine->active.input.quit_request();
	} break;
	case SDL_WINDOWEVENT: {
		switch (ev.window.event) {
		case SDL_WINDOWEVENT_SIZE_CHANGED: {
			int width, height;
			SDL_Vulkan_GetDrawableSize(window, &width, &height);
			engine->active.input.resize({static_cast<u32>(width), static_cast<u32>(height)});
		} break;
		case SDL_WINDOWEVENT_LEAVE: {
			engine->active.input.mouse_position({});
		} break;
		}
	} break;
	case SDL_MOUSEMOTION: {
		if (SDL_GetRelativeMouseMode()) {
			engine->active.input.mouse_relative({static_cast<f32>(ev.motion.xrel), static_cast<f32>(ev.motion.yrel)});
		} else {
			engine->active.input.mouse_position(vec2{static_cast<f32>(ev.motion.x), static_cast<f32>(ev.motion.y)});
		}
	} break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP: {
		Input::MouseButton button;
		switch (ev.button.button) {
		case SDL_BUTTON_LEFT:
			button = Input::MouseButton::Left;
			break;
		case SDL_BUTTON_RIGHT:
			button = Input::MouseButton::Right;
			break;
		case SDL_BUTTON_MIDDLE:
			button = Input::MouseButton::Middle;
			break;
		default:
			return;
		}

		engine->active.input.mouse_button(button, ev.button.state);
	} break;
	case SDL_MOUSEWHEEL: {
		engine->active.input.mouse_scroll(ev.wheel.preciseY);
	} break;
	}
};

std::unique_ptr<Render> SDLPlatform::init_video() {

	window = SDL_CreateWindow(
		"Guidestone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_SetWindowMinimumSize(window, 640, 480);

	Vulkan::Context::Create vulkan_context{
		(PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr(),
		{},
		[window = window](VkInstance instance, VkSurfaceKHR* surface) {
			SDL_Vulkan_CreateSurface(window, instance, surface);
		}};

	unsigned int sdl_vulkan_extensions_count;
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_vulkan_extensions_count, nullptr);
	vulkan_context.extensions.resize(sdl_vulkan_extensions_count);
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_vulkan_extensions_count, vulkan_context.extensions.data());

	return std::make_unique<Vulkan::Render>(vulkan_context);
}

void SDLPlatform::set_relative_mouse(bool enable) { SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE); }

void SDLPlatform::shutdown() { shutdown_semaphore.release(); }

int main(int argc, char* argv[]) {
#ifdef __LINUX__
	SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
#endif

	SDLPlatform platform(argc, argv);

	platform.run();
}
