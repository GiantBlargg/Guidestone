#pragma once

#include "command.hpp"
#include "context.hpp"
#include "device.hpp"
#include "framebuffer.hpp"
#include "render.hpp"
#include "storage.hpp"
#include "swapchain.hpp"
#include <vulkan/vulkan.hpp>

namespace Vulkan {

class Render final : public ::Render {
	const Context context;
	const Device device;

	Framebuffer framebuffer;
	f32 aspect = 0;

	Storage storage;

	Command cmd;

	vk::Pipeline default_pipeline;

	ModelCache models;

  public:
	Render(Context::Create);
	~Render();

	void resize(uvec2 size) override {
		framebuffer.resize(size);
		aspect = static_cast<f32>(size.x) / static_cast<f32>(size.y);
	}
	void renderFrame(FrameInfo) override;
	void setModelCache(const ModelCache&) override;
};

} // namespace Vulkan
