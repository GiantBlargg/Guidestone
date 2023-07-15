#pragma once

#include "model.hpp"
#include "types.hpp"

class Render {
  public:
	virtual void resize(u32 width, u32 height) = 0;
	virtual void renderFrame() = 0;
	virtual void setModelCache(const ModelCache&) = 0;
};
