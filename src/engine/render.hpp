#pragma once

#include "camera.hpp"
#include "model.hpp"
#include "types.hpp"

// This is stuff that could change every render frame

class Render {
  public:
	virtual void resize(u32 width, u32 height) = 0;
	struct FrameInfo {
		const Camera& camera;
	};
	virtual void renderFrame(FrameInfo) = 0;
	virtual void setModelCache(const ModelCache&) = 0;
};
