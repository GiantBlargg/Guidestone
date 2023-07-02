#pragma once

#include <cstdint>

class Render {
  public:
	virtual void resize(uint32_t width, uint32_t height) = 0;
	virtual void renderFrame() = 0;
};