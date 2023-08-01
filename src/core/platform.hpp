#pragma once

#include "active/input.hpp"
#include "render.hpp"

class Platform {
  public:
	// Only call this function during Engine::init
	virtual std::unique_ptr<Render> init_video() = 0;

	virtual void set_relative_mouse(bool enable) = 0;

	virtual void shutdown() = 0;
};
