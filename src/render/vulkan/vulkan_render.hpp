#pragma once

#include "engine/render.hpp"
#include <functional>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

namespace Vulkan {

#if !defined(RENDER_DEBUG)
#if !defined(NDEBUG)
#define RENDER_DEBUG 1
#endif
#endif

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

class VulkanRender : public Render {
	vk::Instance instance;
#if RENDER_DEBUG == 1
	vk::DebugUtilsMessengerEXT debug_messenger;
#endif
	vk::SurfaceKHR surface;

	struct DeviceConfig {
		vk::PhysicalDevice physical_device;
		uint32_t queue_family;
		vk::SurfaceFormatKHR surface_colour_format;
		vk::PresentModeKHR present_mode;
		bool memory_budget;
		bool memory_priority;
	} device_config;
	vk::Device device;
	vk::Queue queue;

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

	vma::Allocator allocator;

	void reconfigureSwapchain();
	uint32_t acquireImage(vk::Semaphore semaphore);

  public:
	VulkanRender(
		PFN_vkGetInstanceProcAddr, std::vector<const char*> vulkan_extensions,
		std::function<vk::SurfaceKHR(vk::Instance)> make_surface);
	~VulkanRender();

	void resize(uint32_t width, uint32_t height) override;
	void renderFrame() override;
};

} // namespace Vulkan
