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

void Storage::update_uniform(const Uniform& uniform, size_t index) {
	vk::DeviceSize offset = (uniform_stride * index);
	memcpy((u8*)(uniform_buffer.ptr) + offset, &uniform, sizeof(Uniform));
	device.allocator.flushAllocation(uniform_buffer, offset, sizeof(Uniform));
}

Storage::Storage(const Device& d) : device(d) {
	{
		vk::DescriptorPoolSize pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);
		vk::DescriptorPoolCreateInfo pool_info({}, 1, pool_size);
		uniform_pool = device->createDescriptorPool(pool_info);

		vk::DescriptorSetLayoutBinding layout_bind(
			0, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex);
		vk::DescriptorSetLayoutCreateInfo layout_info({}, layout_bind);
		uniform_layout = device->createDescriptorSetLayout(layout_info);
		vk::DescriptorSetAllocateInfo set_info(uniform_pool, uniform_layout);
		uniform_set = device->allocateDescriptorSets(set_info).front();
	}
	{
		vk::PipelineLayoutCreateInfo layout_info({}, uniform_layout);
		pipeline_layout = device->createPipelineLayout(layout_info);
	}

	{
		vk::DeviceSize min_stride = device.physical_device.getProperties().limits.minUniformBufferOffsetAlignment;
		uniform_stride = (sizeof(Uniform) + min_stride - 1) & ~(min_stride - 1);

		vk::BufferCreateInfo uniform_info;
		uniform_info.setSize(uniform_stride * RenderCommand::size).setUsage(vk::BufferUsageFlagBits::eUniformBuffer);
		vma::AllocationCreateInfo alloc_info(
			vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
			vma::MemoryUsage::eAuto);
		uniform_buffer.init(device, uniform_info, alloc_info);
	}
	{
		vk::DescriptorBufferInfo desc_buf(uniform_buffer, 0, uniform_stride);
		vk::WriteDescriptorSet write_sets(uniform_set, 0, 0);
		write_sets.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic).setBufferInfo(desc_buf);
		device->updateDescriptorSets(write_sets, {});
	}
}

Storage::~Storage() {
	device->destroy(pipeline_layout);
	device->destroy(uniform_layout);
	device->destroy(uniform_pool);
	uniform_buffer.destroy(device);

	if (vertex_buffer) {
		vertex_buffer.destroy(device);
	}
}

void Storage::bind_buffers(vk::CommandBuffer cmd, size_t index) {
	{
		vk::DeviceSize offset = 0;
		cmd.bindVertexBuffers(0, vertex_buffer.buffer, offset);
	}
	{
		vk::DeviceSize offset = index * uniform_stride;
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, uniform_set, offset);
	}
}

} // namespace Vulkan
