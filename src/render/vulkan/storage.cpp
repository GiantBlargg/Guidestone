#include "storage.hpp"
#include "command.hpp"

namespace Vulkan {

void ImageAllocation::init(
	const Device& device, const vk::ImageCreateInfo& image_info, const vma::AllocationCreateInfo& alloc_info) {
	destroy(device);
	auto r = device.allocator.createImage(image_info, alloc_info);
	image = r.first;
	alloc = r.second;
}
void ImageAllocation::init(
	const Device& device, const vk::ImageCreateInfo& image_info, const vma::AllocationCreateInfo& alloc_info,
	vk::ImageViewCreateInfo& view_info) {
	init(device, image_info, alloc_info);
	view_info.setImage(image);
	view = device->createImageView(view_info);
}

void ImageAllocation::destroy(const Device& device) {
	if (view)
		device->destroy(view);
	if (image)
		device.allocator.destroyImage(image, alloc);
}

void BufferAllocation::init(
	const Device& device, const vk::BufferCreateInfo& buffer_info, const vma::AllocationCreateInfo& alloc_info) {
	destroy(device);
	auto r = device.allocator.createBuffer(buffer_info, alloc_info);
	buffer = r.first;
	alloc = r.second;
	ptr = device.allocator.getAllocationInfo(alloc).pMappedData;
}
void BufferAllocation::destroy(const Device& device) {
	if (buffer)
		device.allocator.destroyBuffer(buffer, alloc);
}

void Storage::resize_frame(vk::Extent2D size) {
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

void Storage::update_vertex_buffer(const void* src, size_t n) {
	vk::BufferCreateInfo buffer_info({}, n, vk::BufferUsageFlagBits::eVertexBuffer);
	vma::AllocationCreateInfo alloc_info(
		vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
		vma::MemoryUsage::eAutoPreferDevice);
	vertex_buffer.init(device, buffer_info, alloc_info);
	memcpy(vertex_buffer, src, n);
	device.allocator.flushAllocation(vertex_buffer, 0, n);
}

void Storage::update_uniform(const Uniform& uniform, size_t index) {
	vk::DeviceSize offset = (uniform_stride * index);
	memcpy((u8*)(uniform_buffer.ptr) + offset, &uniform, sizeof(Uniform));
	device.allocator.flushAllocation(uniform_buffer, offset, sizeof(Uniform));
}

Storage::Storage(const Device& d) : device(d) {
	vk::DeviceSize min_stride = device.physical_device.getProperties().limits.minUniformBufferOffsetAlignment;
	uniform_stride = (sizeof(Uniform) + min_stride - 1) & ~(min_stride - 1);

	vk::BufferCreateInfo uniform_info;
	uniform_info.setSize(uniform_stride * RenderCommand::size).setUsage(vk::BufferUsageFlagBits::eUniformBuffer);
	vma::AllocationCreateInfo alloc_info(
		vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
		vma::MemoryUsage::eAuto);
	uniform_buffer.init(device, uniform_info, alloc_info);
}

Storage::~Storage() {
	uniform_buffer.destroy(device);

	if (depth_buffer) {
		depth_buffer.destroy(device);
	}

	if (vertex_buffer) {
		vertex_buffer.destroy(device);
	}
}

void Storage::start_render(vk::CommandBuffer cmd, Swapchain::Image image) {
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

		cmd.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
	}

	{
		vk::Viewport viewport(0, 0, frame_extent.width, frame_extent.height, 0, 1);
		cmd.setViewport(0, viewport);
		vk::Rect2D scissors({0, 0}, frame_extent);
		cmd.setScissor(0, scissors);

		vk::RenderingAttachmentInfo colour_attachment(image, vk::ImageLayout::eColorAttachmentOptimal);
		colour_attachment.setLoadOp(vk::AttachmentLoadOp::eDontCare).setStoreOp(vk::AttachmentStoreOp::eStore);
#if 1
		vk::ClearValue clear_value(vk::ClearColorValue(std::array<float, 4>({{0.0, 0.3, 0.8, 1.0}})));
		colour_attachment.setLoadOp(vk::AttachmentLoadOp::eClear).setClearValue(clear_value);
#endif
		vk::RenderingAttachmentInfo depth_attachment(depth_buffer.view, vk::ImageLayout::eDepthAttachmentOptimal);
		depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setClearValue(vk::ClearValue(vk::ClearDepthStencilValue(0)))
			.setStoreOp(vk::AttachmentStoreOp::eDontCare);
		vk::RenderingInfo rendering_info({}, scissors, 1, 0, colour_attachment, &depth_attachment);
		cmd.beginRendering(rendering_info);
	}

	vk::DeviceSize offset = 0;
	cmd.bindVertexBuffers(0, vertex_buffer.buffer, offset);
}

void Storage::end_render(vk::CommandBuffer cmd, Swapchain::Image image) {
	cmd.endRendering();

	{
		vk::ImageMemoryBarrier2 image_barrier(
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, {}, vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR, device.graphics_queue.family, device.graphics_queue.family, image,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
		cmd.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
	}
}

} // namespace Vulkan
