#include "storage.hpp"

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

Storage::Storage(const Device& d) : device(d) {}

Storage::~Storage() {
	if (depth_buffer) {
		depth_buffer.destroy(device);
	}

	if (vertex_buffer) {
		vertex_buffer.destroy(device);
	}
}

} // namespace Vulkan
