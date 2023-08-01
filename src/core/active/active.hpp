#pragma once

#include "camera.hpp"
#include "input.hpp"
#include <thread>

class Engine;

class Active {
	Engine& engine;

	std::thread thread;
	void thread_func();
	std::binary_semaphore start_signal{0};

  public:
	Active(Engine&);
	void start();
	~Active();

	Input input;
	CameraSystem camera_system;
};
