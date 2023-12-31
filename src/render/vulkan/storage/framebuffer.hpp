#pragma once

#include "command.hpp"
#include "device.hpp"
#include "math.hpp"
#include "storage.hpp"
#include "swapchain.hpp"

namespace Vulkan {

class Framebuffer {
	const Device& device;
	Swapchain swapchain;

	Swapchain::Image image;

	vk::Extent2D frame_extent;
	void resize_frame(vk::Extent2D);

	ImageAllocation depth_buffer;

	std::array<vk::Semaphore, Command::size> acquire_semaphores;
	std::array<vk::Semaphore, Command::size> present_semaphores;

  public:
	Framebuffer(const vk::SurfaceKHR& s, const Device& d);
	~Framebuffer();

	void resize(uvec2);
	void start_rendering(Command&);
	void present(Command&);
};

} // namespace Vulkan
