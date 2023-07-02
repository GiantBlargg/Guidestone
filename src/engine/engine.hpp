#pragma once

#include "render.hpp"
#include <flecs.h>

class Engine {
	flecs::world universe;

	Render* render;

  public:
	constexpr static double tick_interval = 1.0 / 16.0;

	void init(Render*);
	void startGame();

	void tickUpdate();
	void update(double delta);
};
