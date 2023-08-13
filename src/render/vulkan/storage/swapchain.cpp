#include "swapchain.hpp"

namespace Vulkan {

Swapchain::~Swapchain() {
	for (auto& image_view : image_views)
		device->destroyImageView(image_view);

	device->destroySwapchainKHR(swapchain);
}

void Swapchain::reconfigureSwapchain() {
	update = false;

	// Seems to work without
	device->waitIdle();

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

		swapchain = device->createSwapchainKHR(vk::SwapchainCreateInfoKHR(
			{}, surface, image_count, device.surface_format.format, device.surface_format.colorSpace, extent, 1,
			vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, {},
			vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, device.present_mode,
			true, old_swapchain));

		if (old_swapchain)
			device->destroySwapchainKHR(old_swapchain);
	}
	images = device->getSwapchainImagesKHR(swapchain);

	for (auto& image_view : image_views)
		device->destroyImageView(image_view);
	image_views.resize(images.size());
	for (size_t i = 0; i < image_views.size(); i++) {
		image_views[i] = device->createImageView(vk::ImageViewCreateInfo(
			{}, images[i], vk::ImageViewType::e2D, device.surface_format.format, {},
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
	}
}

Swapchain::Image Swapchain::acquireImage(vk::Semaphore semaphore) {
	if (update)
		reconfigureSwapchain();

	u32 index;
	auto result = device->acquireNextImageKHR(swapchain, UINT64_MAX, semaphore, nullptr, &index);
	vk::resultCheck(
		result, "vk::Device::acquireNextImageKHR",
		{vk::Result::eSuccess, vk::Result::eSuboptimalKHR, vk::Result::eErrorOutOfDateKHR});
	if (result != vk::Result::eSuccess) {
		update = true;
		if (result != vk::Result::eSuboptimalKHR)
			return acquireImage(semaphore);
	}
	return Swapchain::Image{index, images[index], image_views[index]};
}

void Swapchain::present(
	const vk::ArrayProxyNoTemporaries<const vk::Semaphore>& waitSemaphores, const Swapchain::Image& image) {
	vk::PresentInfoKHR present_info(waitSemaphores, swapchain, image.index);
	vk::Result result = device.graphics_queue.queue.presentKHR(&present_info);
	vk::resultCheck(
		result, "vk::Queue::presentKHR",
		{vk::Result::eSuccess, vk::Result::eSuboptimalKHR, vk::Result::eErrorOutOfDateKHR});
	if (result != vk::Result::eSuccess)
		update = true;
}

} // namespace Vulkan
