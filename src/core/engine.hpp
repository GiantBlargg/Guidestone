#pragma once

#include "active/active.hpp"
#include "platform.hpp"
#include "render.hpp"

class Engine {
  public:
	Platform& platform;

	Active active;

	std::unique_ptr<Render> render;

  private:
	Engine(Engine&) = delete;
	Engine(Engine&&) = delete;

  public:
	Engine(Platform&);
	void init();
	~Engine() {}
	void startGame();
};
