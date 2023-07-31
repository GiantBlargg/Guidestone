#pragma once

#include "input.hpp"
#include "platform.hpp"
#include "render.hpp"
#include <thread>

class Engine {
  public:
	Platform& platform;

	Input input;

	std::unique_ptr<Render> render;

	CameraSystem camera_system;

  private:
	std::thread interactive_thread;
	void interactive_thread_func();

	Engine(Engine&) = delete;
	Engine(Engine&&) = delete;

  public:
	Engine(Platform&);
	void init();
	~Engine();
	void startGame();
};
