#include "uniform.hpp"

namespace Vulkan {

UniformBuffer::UniformBuffer(const Device& d) : device(d) {
	{
		vk::DescriptorSetLayoutBinding layout_bind(
			0, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex);
		vk::DescriptorSetLayoutCreateInfo layout_info({}, layout_bind);
		uniform_layout = device->createDescriptorSetLayout(layout_info);
	}
	{
		vk::DescriptorPoolSize pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);
		vk::DescriptorPoolCreateInfo pool_info({}, 1, pool_size);
		uniform_pool = device->createDescriptorPool(pool_info);
	}
	{
		vk::DescriptorSetAllocateInfo set_info(uniform_pool, uniform_layout);
		uniform_set = device->allocateDescriptorSets(set_info).front();
	}

	{
		vk::DeviceSize min_stride = device.physical_device.getProperties().limits.minUniformBufferOffsetAlignment;
		uniform_stride = (sizeof(Uniform) + min_stride - 1) & ~(min_stride - 1);
	}
	{
		vk::BufferCreateInfo uniform_info;
		uniform_info.setSize(uniform_stride * Command::size).setUsage(vk::BufferUsageFlagBits::eUniformBuffer);
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

UniformBuffer::~UniformBuffer() {
	device->destroy(uniform_layout);
	device->destroy(uniform_pool);
	uniform_buffer.destroy(device);
}

std::pair<vk::DescriptorSet, vk::DeviceSize> UniformBuffer::update_uniform(const Uniform& uniform, size_t index) {
	vk::DeviceSize offset = (uniform_stride * index);

	memcpy((u8*)(uniform_buffer.ptr) + offset, &uniform, sizeof(Uniform));
	device.allocator.flushAllocation(uniform_buffer, offset, sizeof(Uniform));
	return {uniform_set, offset};
}
} // namespace Vulkan
