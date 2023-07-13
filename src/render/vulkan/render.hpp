#pragma once

#include "command.hpp"
#include "context.hpp"
#include "device.hpp"
#include "engine/render.hpp"
#include "swapchain.hpp"
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Vulkan {

struct ImageAllocation {
	vk::Image image;
	vma::Allocation alloc;

	ImageAllocation& operator=(const std::pair<vk::Image, vma::Allocation> p) {
		image = p.first;
		alloc = p.second;
		return *this;
	}

	operator vk::Image() { return image; }

	void destroy(vma::Allocator allocator) { return allocator.destroyImage(image, alloc); }
};

class Render : public ::Render {
	const Context context;
	const Device device;
	Swapchain swapchain;

	struct PerFrame {
		vk::Semaphore acquire_semaphore;
		vk::Semaphore rendering_semaphore;
	};
	Command<3, PerFrame> render_cmd;

	vk::Extent2D frame_extent;
	ImageAllocation depth_buffer;
	vk::ImageView depth_view;
	void resize_frame();

  public:
	Render(Context::Create);
	~Render();

	void resize(uint32_t width, uint32_t height) override { swapchain.set_extent(vk::Extent2D{width, height}); }
	void renderFrame() override;
};

} // namespace Vulkan
