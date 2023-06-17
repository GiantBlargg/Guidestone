#pragma once

#include <functional>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Render {

#if !defined(RENDER_DEBUG)
#if !defined(NDEBUG)
#define RENDER_DEBUG 1
#endif
#endif

class Render {
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
	} device_config;
	vk::Device device;
	vk::Queue queue;

	bool update_swapchain = true;
	vk::Extent2D surface_extent;
	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> images;
	std::vector<vk::ImageView> image_views;

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
	Render(
		PFN_vkGetInstanceProcAddr, std::vector<const char*> vulkan_extensions,
		std::function<vk::SurfaceKHR(vk::Instance)> make_surface);
	~Render();

	void resize(uint32_t width, uint32_t height);
	void renderFrame();
};

} // namespace Render
extern Render::Render* render;