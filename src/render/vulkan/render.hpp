#pragma once

#include "context.hpp"
#include "device.hpp"
#include "engine/render.hpp"
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

	bool update_swapchain = true;
	vk::Extent2D surface_extent;
	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> images;
	std::vector<vk::ImageView> image_views;
	ImageAllocation depth_buffer;
	vk::ImageView depth_view;

	constexpr static int frame_concurrency = 2;
	struct PerFrame {
		vk::CommandPool command_pool;
		vk::CommandBuffer command_buffer;
		vk::Fence submission_fence;
		vk::Semaphore acquire_semaphore;
		vk::Semaphore rendering_semaphore;
	} per_frame[frame_concurrency];
	size_t frame_index = -1;

	void reconfigureSwapchain();
	uint32_t acquireImage(vk::Semaphore semaphore);

  public:
	Render(Context::Create);
	~Render();

	void resize(uint32_t width, uint32_t height) override;
	void renderFrame() override;
};

} // namespace Vulkan
