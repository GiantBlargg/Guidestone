#pragma once

#include "device.hpp"
#include "engine/types.hpp"
#include <vulkan/vulkan.hpp>

namespace Vulkan {

class Swapchain {
	vk::SurfaceKHR surface;
	const Device& device;
	bool update = true;
	vk::Extent2D extent;
	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> images;
	std::vector<vk::ImageView> image_views;

	void reconfigureSwapchain();

  public:
	Swapchain(const Swapchain&) = delete;
	Swapchain(const vk::SurfaceKHR& s, const Device& d) : surface(s), device(d) {}
	~Swapchain();

	vk::Extent2D get_extent() { return extent; }
	// Due to vulkan limits, may not actually set to this value
	void set_extent(vk::Extent2D e) {
		if (extent == e) {
			return;
		}
		extent = e;
		update = true;
	}

	struct Image {
		u32 index;
		vk::Image image;
		vk::ImageView view;

		operator vk::Image() const { return image; }
		operator vk::ImageView() const { return view; }
	};

	Image acquireImage(vk::Semaphore semaphore);
	void present(const vk::ArrayProxyNoTemporaries<const vk::Semaphore>& waitSemaphores, const Image& image);
};

} // namespace Vulkan
