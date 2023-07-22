#pragma once

#include "command.hpp"
#include "context.hpp"
#include "device.hpp"
#include "engine/render.hpp"
#include "storage.hpp"
#include "swapchain.hpp"
#include <vulkan/vulkan.hpp>

namespace Vulkan {

class Render : public ::Render {
	const Context context;
	const Device device;
	Swapchain swapchain;

	Storage storage;

	f32 aspect = 0;

	RenderCommand render_cmd;

	vk::Pipeline default_pipeline;

  public:
	Render(Context::Create);
	~Render();

	void resize(uint32_t width, uint32_t height) override { swapchain.set_extent(vk::Extent2D{width, height}); }
	void renderFrame(FrameInfo) override;
	void setModelCache(const ModelCache&) override;
};

} // namespace Vulkan
