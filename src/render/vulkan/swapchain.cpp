#include "swapchain.hpp"

namespace Vulkan {

Swapchain::~Swapchain() {
	for (auto& image_view : image_views)
		device.device.destroyImageView(image_view);

	device.device.destroySwapchainKHR(swapchain);
}

void Swapchain::reconfigureSwapchain() {
	update = false;

	// Seems to work without
	device.device.waitIdle();

	vk::SurfaceCapabilitiesKHR caps = device.physical_device.getSurfaceCapabilitiesKHR(surface);
	if (caps.currentExtent != vk::Extent2D(0xFFFFFFFF, 0xFFFFFFFF) && caps.currentExtent != vk::Extent2D(0, 0)) {
		extent = caps.currentExtent;
	} else {
		if (extent.width < caps.minImageExtent.width)
			extent.width = caps.minImageExtent.width;
		if (extent.height < caps.minImageExtent.height)
			extent.height = caps.minImageExtent.height;
		if (extent.width > caps.maxImageExtent.width)
			extent.width = caps.maxImageExtent.width;
		if (extent.height > caps.maxImageExtent.height)
			extent.height = caps.maxImageExtent.height;
	}

	uint32_t image_count = 3;
	if (image_count < caps.minImageCount)
		image_count = caps.minImageCount;
	if (caps.maxImageCount != 0 && image_count > caps.maxImageCount)
		image_count = caps.maxImageCount;

	{
		vk::SwapchainKHR old_swapchain = swapchain;

		swapchain = device.device.createSwapchainKHR(vk::SwapchainCreateInfoKHR(
			{}, surface, image_count, device.surface_format.format, device.surface_format.colorSpace, extent, 1,
			vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, {},
			vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, device.present_mode,
			true, old_swapchain));

		if (old_swapchain)
			device.device.destroySwapchainKHR(old_swapchain);
	}
	images = device.device.getSwapchainImagesKHR(swapchain);

	for (auto& image_view : image_views)
		device.device.destroyImageView(image_view);
	image_views.resize(images.size());
	for (size_t i = 0; i < image_views.size(); i++) {
		image_views[i] = device.device.createImageView(vk::ImageViewCreateInfo(
			{}, images[i], vk::ImageViewType::e2D, device.surface_format.format, {},
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
	}
}

Swapchain::Image Swapchain::acquireImage(vk::Semaphore semaphore) {
	if (update)
		reconfigureSwapchain();

	auto image_index_result = device.device.acquireNextImageKHR(swapchain, UINT64_MAX, semaphore, nullptr);
	if (image_index_result.result != vk::Result::eSuccess) {
		update = true;
		if (image_index_result.result != vk::Result::eSuboptimalKHR)
			return acquireImage(semaphore);
	}
	u32 index = image_index_result.value;
	return Swapchain::Image{index, images[index], image_views[index]};
}

void Swapchain::present(
	const vk::ArrayProxyNoTemporaries<const vk::Semaphore>& waitSemaphores, const Swapchain::Image& image) {
	vk::Result result =
		device.graphics_queue.queue.presentKHR(vk::PresentInfoKHR(waitSemaphores, swapchain, image.index));
	if (result != vk::Result::eSuccess)
		update = true;
}

} // namespace Vulkan
