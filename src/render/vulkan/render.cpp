#include "render.hpp"

#include "engine/log.hpp"

namespace Vulkan {

Render::Render(Context::Create c) : context(c), device(context) {

	reconfigureSwapchain();

	for (int i = 0; i < frame_concurrency; i++) {
		PerFrame& f = per_frame[i];
		f.command_pool = device.device.createCommandPool(vk::CommandPoolCreateInfo({}, device.config.queue_family));
		f.command_buffer = device.device
							   .allocateCommandBuffers(
								   vk::CommandBufferAllocateInfo(f.command_pool, vk::CommandBufferLevel::ePrimary, 1))
							   .front();
		f.submission_fence = device.device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
		f.acquire_semaphore = device.device.createSemaphore({});
		f.rendering_semaphore = device.device.createSemaphore({});
	}
}

Render::~Render() {
	device.device.waitIdle();

	for (int i = 0; i < frame_concurrency; i++) {
		PerFrame& f = per_frame[i];
		device.device.destroyFence(f.submission_fence);
		device.device.destroySemaphore(f.acquire_semaphore);
		device.device.destroySemaphore(f.rendering_semaphore);
		device.device.destroyCommandPool(f.command_pool);
	}

	for (auto& image_view : image_views)
		device.device.destroyImageView(image_view);

	device.device.destroyImageView(depth_view);
	depth_buffer.destroy(device.allocator);

	device.device.destroySwapchainKHR(swapchain);
}

void Render::reconfigureSwapchain() {
	update_swapchain = false;

	// Seems to work without
	device.device.waitIdle();

	vk::SurfaceCapabilitiesKHR caps = device.config.physical_device.getSurfaceCapabilitiesKHR(context.surface);
	if (caps.currentExtent != vk::Extent2D(0xFFFFFFFF, 0xFFFFFFFF) && caps.currentExtent != vk::Extent2D(0, 0)) {
		surface_extent = caps.currentExtent;
	} else {
		if (surface_extent.width < caps.minImageExtent.width)
			surface_extent.width = caps.minImageExtent.width;
		if (surface_extent.height < caps.minImageExtent.height)
			surface_extent.height = caps.minImageExtent.height;
		if (surface_extent.width > caps.maxImageExtent.width)
			surface_extent.width = caps.maxImageExtent.width;
		if (surface_extent.height > caps.maxImageExtent.height)
			surface_extent.height = caps.maxImageExtent.height;
	}

	uint32_t image_count = 3;
	if (image_count < caps.minImageCount)
		image_count = caps.minImageCount;
	if (caps.maxImageCount != 0 && image_count > caps.maxImageCount)
		image_count = caps.maxImageCount;

	{
		vk::SwapchainKHR old_swapchain = swapchain;

		swapchain = device.device.createSwapchainKHR(vk::SwapchainCreateInfoKHR(
			{}, context.surface, image_count, device.config.surface_colour_format.format,
			device.config.surface_colour_format.colorSpace, surface_extent, 1, vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive, {}, vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true, old_swapchain));

		if (old_swapchain)
			device.device.destroySwapchainKHR(old_swapchain);
	}
	images = device.device.getSwapchainImagesKHR(swapchain);

	for (auto& image_view : image_views)
		device.device.destroyImageView(image_view);
	image_views.resize(images.size());
	for (size_t i = 0; i < image_views.size(); i++) {
		image_views[i] = device.device.createImageView(vk::ImageViewCreateInfo(
			{}, images[i], vk::ImageViewType::e2D, device.config.surface_colour_format.format, {},
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
	}

	if (depth_view) {
		device.device.destroyImageView(depth_view);
		depth_buffer.destroy(device.allocator);
	}

	{
		vk::Format depth_format = vk::Format::eD32Sfloat;
		vk::ImageCreateInfo depth_info(
			{}, vk::ImageType::e2D, depth_format, vk::Extent3D(surface_extent.width, surface_extent.height, 1), 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::SharingMode::eExclusive, device.config.queue_family, vk::ImageLayout::eUndefined);
		vma::AllocationCreateInfo alloc_info;
		alloc_info.setUsage(vma::MemoryUsage::eGpuOnly).setPriority(1);
		depth_buffer = device.allocator.createImage(depth_info, alloc_info);
		depth_view = device.device.createImageView(vk::ImageViewCreateInfo(
			{}, depth_buffer, vk::ImageViewType::e2D, depth_format, {},
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)));
	}
}

void Render::resize(uint32_t width, uint32_t height) {
	vk::Extent2D new_extent{width, height};
	if (surface_extent == new_extent)
		return;

	update_swapchain = true;
	surface_extent = new_extent;
}

uint32_t Render::acquireImage(vk::Semaphore semaphore) {
	if (update_swapchain)
		reconfigureSwapchain();

	auto image_index_result = device.device.acquireNextImageKHR(swapchain, UINT64_MAX, semaphore, nullptr);
	if (image_index_result.result != vk::Result::eSuccess) {
		update_swapchain = true;
		if (image_index_result.result != vk::Result::eSuboptimalKHR)
			return acquireImage(semaphore);
	}
	return image_index_result.value;
}

void Render::renderFrame() {
	frame_index++;
	auto& frame = per_frame[frame_index % frame_concurrency];

	auto fence_result = device.device.waitForFences(frame.submission_fence, false, UINT64_MAX);
	if (fence_result != vk::Result::eSuccess) {
		throw new vk::LogicError(to_string(fence_result));
	}
	device.device.resetFences(frame.submission_fence);
	device.device.resetCommandPool(frame.command_pool);

	uint32_t image_index = acquireImage(frame.acquire_semaphore);

	{
		frame.command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		{
			std::vector<vk::ImageMemoryBarrier2> image_barrier;
			image_barrier.push_back(vk::ImageMemoryBarrier2(
				vk::PipelineStageFlagBits2::eColorAttachmentOutput, {},
				vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, device.config.queue_family,
				device.config.queue_family, images[image_index],
				vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
			image_barrier.push_back(vk::ImageMemoryBarrier2(
				{}, {}, vk::PipelineStageFlagBits2::eEarlyFragmentTests,
				vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::ImageLayout::eUndefined,
				vk::ImageLayout::eDepthAttachmentOptimal, device.config.queue_family, device.config.queue_family,
				depth_buffer, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)));

			frame.command_buffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
		}

		{
			vk::Viewport viewport(0, 0, surface_extent.width, surface_extent.height, 0, 1);
			frame.command_buffer.setViewport(0, viewport);
			vk::Rect2D scissors({0, 0}, surface_extent);
			frame.command_buffer.setScissor(0, scissors);

			vk::RenderingAttachmentInfo colour_attachment(
				image_views[image_index], vk::ImageLayout::eColorAttachmentOptimal);
			colour_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
#if 1
			vk::ClearValue clear_value(vk::ClearColorValue(std::array<float, 4>({{0.0, 0.3, 0.8, 1.0}})));
			colour_attachment.setLoadOp(vk::AttachmentLoadOp::eClear).setClearValue(clear_value);
#else
			colour_attachment.setLoadOp(vk::AttachmentLoadOp::eDontCare);
#endif
			vk::RenderingAttachmentInfo depth_attachment(depth_view, vk::ImageLayout::eDepthAttachmentOptimal);
			depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setClearValue(vk::ClearValue(vk::ClearDepthStencilValue(0)));
			vk::RenderingInfo rendering_info({}, scissors, 1, 0, colour_attachment);
			frame.command_buffer.beginRendering(rendering_info);
		}

		frame.command_buffer.endRendering();

		{
			vk::ImageMemoryBarrier2 image_barrier(
				vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
				vk::PipelineStageFlagBits2::eColorAttachmentOutput, {}, vk::ImageLayout::eColorAttachmentOptimal,
				vk::ImageLayout::ePresentSrcKHR, device.config.queue_family, device.config.queue_family,
				images[image_index], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
			frame.command_buffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
		}

		frame.command_buffer.end();
	}

	vk::SemaphoreSubmitInfo acquire_semaphore_info(
		frame.acquire_semaphore, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	vk::CommandBufferSubmitInfo cmd_info(frame.command_buffer);
	vk::SemaphoreSubmitInfo rendering_semaphore_info(
		frame.rendering_semaphore, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	vk::SubmitInfo2 submit_info({}, acquire_semaphore_info, cmd_info, rendering_semaphore_info);
	device.queue.submit2(submit_info, frame.submission_fence);

	vk::Result result = device.queue.presentKHR(vk::PresentInfoKHR(frame.rendering_semaphore, swapchain, image_index));
	if (result != vk::Result::eSuccess)
		update_swapchain = true;
}

} // namespace Vulkan
