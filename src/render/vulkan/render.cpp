#include "render.hpp"

#include "engine/log.hpp"

namespace Vulkan {

Render::Render(Context::Create c)
	: context(c), device(context), swapchain(context.surface, device), render_cmd(device, device.graphics_queue) {
	render_cmd.forEach([device = device.device](PerFrame& f) {
		f.acquire_semaphore = device.createSemaphore({});
		f.rendering_semaphore = device.createSemaphore({});
	});
}

Render::~Render() {
	device.device.waitIdle();

	if (depth_view) {
		device.device.destroyImageView(depth_view);
		depth_buffer.destroy(device.allocator);
	}

	render_cmd.forEach([device = device.device](PerFrame& f) {
		device.destroySemaphore(f.acquire_semaphore);
		device.destroySemaphore(f.rendering_semaphore);
	});
}

void Render::resize_frame() {
	if (depth_view) {
		device.device.destroyImageView(depth_view);
		depth_buffer.destroy(device.allocator);
	}

	{
		vk::ImageCreateInfo depth_info(
			{}, vk::ImageType::e2D, device.depth_format, vk::Extent3D(frame_extent.width, frame_extent.height, 1), 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::SharingMode::eExclusive, device.graphics_queue.family, vk::ImageLayout::eUndefined);
		vma::AllocationCreateInfo alloc_info;
		alloc_info.setUsage(vma::MemoryUsage::eGpuOnly).setPriority(1);
		depth_buffer = device.allocator.createImage(depth_info, alloc_info);
		depth_view = device.device.createImageView(vk::ImageViewCreateInfo(
			{}, depth_buffer, vk::ImageViewType::e2D, device.depth_format, {},
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)));
	}
}

void Render::renderFrame() {

	auto& cmd = render_cmd.get();

	auto image = swapchain.acquireImage(cmd.data.acquire_semaphore);

	if (frame_extent != swapchain.get_extent()) [[unlikely]] {
		frame_extent = swapchain.get_extent();
		resize_frame();
	}

	{
		std::vector<vk::ImageMemoryBarrier2> image_barrier;
		image_barrier.push_back(vk::ImageMemoryBarrier2(
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal, device.graphics_queue.family, device.graphics_queue.family, image,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
		image_barrier.push_back(vk::ImageMemoryBarrier2(
			{}, {}, vk::PipelineStageFlagBits2::eEarlyFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal, device.graphics_queue.family,
			device.graphics_queue.family, depth_buffer,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)));

		cmd->pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
	}

	{
		vk::Viewport viewport(0, 0, frame_extent.width, frame_extent.height, 0, 1);
		cmd->setViewport(0, viewport);
		vk::Rect2D scissors({0, 0}, frame_extent);
		cmd->setScissor(0, scissors);

		vk::RenderingAttachmentInfo colour_attachment(image, vk::ImageLayout::eColorAttachmentOptimal);
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
		cmd->beginRendering(rendering_info);
	}

	cmd->endRendering();

	{
		vk::ImageMemoryBarrier2 image_barrier(
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, {}, vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR, device.graphics_queue.family, device.graphics_queue.family, image,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
		cmd->pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
	}

	vk::SemaphoreSubmitInfo acquire_semaphore_info(
		cmd.data.acquire_semaphore, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	vk::SemaphoreSubmitInfo rendering_semaphore_info(
		cmd.data.rendering_semaphore, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	render_cmd.submit(cmd, acquire_semaphore_info, rendering_semaphore_info);

	swapchain.present(cmd.data.rendering_semaphore, image);
}

} // namespace Vulkan
