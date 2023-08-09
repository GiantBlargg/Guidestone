#include "framebuffer.hpp"

namespace Vulkan {

Framebuffer::Framebuffer(const vk::SurfaceKHR& s, const Device& d) : device(d), swapchain(s, device) {}
Framebuffer::~Framebuffer() {
	if (depth_buffer) {
		depth_buffer.destroy(device);
	}
}

void Framebuffer::resize(uvec2 size) { swapchain.set_extent(vk::Extent2D{size.x, size.y}); }

void Framebuffer::resize_frame(vk::Extent2D size) {
	frame_extent = size;

	vk::ImageCreateInfo depth_info;
	depth_info.setImageType(vk::ImageType::e2D)
		.setFormat(device.depth_format)
		.setExtent(vk::Extent3D(frame_extent.width, frame_extent.height, 1))
		.setMipLevels(1)
		.setArrayLayers(1)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
	vma::AllocationCreateInfo alloc_info;
	alloc_info.setUsage(vma::MemoryUsage::eAutoPreferDevice).setPriority(1);
	vk::ImageViewCreateInfo view_info;
	view_info.setViewType(vk::ImageViewType::e2D)
		.setFormat(device.depth_format)
		.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth)
		.setLevelCount(1)
		.setLayerCount(1);
	depth_buffer.init(device, depth_info, alloc_info, view_info);
}

void Framebuffer::start_rendering(RenderCommand::Instance& cmd) {
	image = swapchain.acquireImage(cmd.acquire_semaphore);

	if (frame_extent != swapchain.get_extent()) [[unlikely]] {
		resize_frame(swapchain.get_extent());
	}

	{
		std::vector<vk::ImageMemoryBarrier2> image_barrier(2);
		image_barrier[0]
			.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
			.setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
			.setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
			.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setImage(image)
			.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLevelCount(1)
			.setLayerCount(1);
		image_barrier[1]
			.setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests)
			.setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
			.setNewLayout(vk::ImageLayout::eDepthAttachmentOptimal)
			.setImage(depth_buffer)
			.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth)
			.setLevelCount(1)
			.setLayerCount(1);

		cmd->pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
	}

	{
		vk::Viewport viewport(0, 0, frame_extent.width, frame_extent.height, 0, 1);
		cmd->setViewport(0, viewport);
		vk::Rect2D scissors({0, 0}, frame_extent);
		cmd->setScissor(0, scissors);

		vk::RenderingAttachmentInfo colour_attachment(image, vk::ImageLayout::eColorAttachmentOptimal);
		colour_attachment.setLoadOp(vk::AttachmentLoadOp::eDontCare).setStoreOp(vk::AttachmentStoreOp::eStore);

		vk::RenderingAttachmentInfo depth_attachment(depth_buffer.view, vk::ImageLayout::eDepthAttachmentOptimal);
		depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setClearValue(vk::ClearValue(vk::ClearDepthStencilValue(0)))
			.setStoreOp(vk::AttachmentStoreOp::eDontCare);

		vk::RenderingInfo rendering_info({}, scissors, 1, 0, colour_attachment, &depth_attachment);
		cmd->beginRendering(rendering_info);
	}
}

void Framebuffer::present(RenderCommand& render_cmd, RenderCommand::Instance& cmd) {
	cmd->endRendering();

	{
		vk::ImageMemoryBarrier2 image_barrier(
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, {}, vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR, device.graphics_queue.family, device.graphics_queue.family, image,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
		cmd->pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
	}

	render_cmd.submit(cmd);

	swapchain.present(cmd.rendering_semaphore, image);
}

} // namespace Vulkan
