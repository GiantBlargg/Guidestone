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
	if (vertex_buffer) {
		vertex_buffer.destroy(device);
	}
}

void Storage::bind_buffers(vk::CommandBuffer cmd, size_t index) {
	vk::DeviceSize offset = 0;
	cmd.bindVertexBuffers(0, vertex_buffer.buffer, offset);
}

} // namespace Vulkan
