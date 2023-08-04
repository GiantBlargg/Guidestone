#pragma once

#include "command.hpp"
#include "context.hpp"
#include "device.hpp"
#include "render.hpp"
#include "storage.hpp"
#include "swapchain.hpp"
#include <vulkan/vulkan.hpp>

namespace Vulkan {

class Render final : public ::Render {
	const Context context;
	const Device device;
	Swapchain swapchain;

	Storage storage;

	f32 aspect = 0;

	RenderCommand render_cmd;

	vk::Pipeline default_pipeline;

	ModelCache models;

  public:
	Render(Context::Create);
	~Render();

	void resize(uvec2 size) override { swapchain.set_extent(vk::Extent2D{size.x, size.y}); }
	void renderFrame(FrameInfo) override;
	void setModelCache(const ModelCache&) override;
};

} // namespace Vulkan
