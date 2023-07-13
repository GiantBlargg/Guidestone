#pragma once

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

	vk::Extent2D frame_extent;
	ImageAllocation depth_buffer;
	vk::ImageView depth_view;
	void resize_frame();

	constexpr static int frame_concurrency = 2;
	struct PerFrame {
		vk::CommandPool command_pool;
		vk::CommandBuffer command_buffer;
		vk::Fence submission_fence;
		vk::Semaphore acquire_semaphore;
		vk::Semaphore rendering_semaphore;
	} per_frame[frame_concurrency];
	size_t frame_index = -1;

  public:
	Render(Context::Create);
	~Render();

	void resize(uint32_t width, uint32_t height) override { swapchain.set_extent(vk::Extent2D{width, height}); }
	void renderFrame() override;
};

} // namespace Vulkan
