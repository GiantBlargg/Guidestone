#pragma once

#include "command.hpp"
#include "math.hpp"
#include "storage.hpp"
#include <vulkan/vulkan.hpp>

namespace Vulkan {

struct Uniform {
	mat4 camera; // Projection * View
};

class UniformBuffer {
	const Device& device;

	vk::DescriptorPool uniform_pool;
	vk::DescriptorSet uniform_set;
	BufferAllocation uniform_buffer;
	vk::DeviceSize uniform_stride;

  public:
	vk::DescriptorSetLayout uniform_layout;
	UniformBuffer(const Device&);
	~UniformBuffer();
	std::pair<vk::DescriptorSet, vk::DeviceSize> update_uniform(const Uniform&, size_t index);
};

} // namespace Vulkan
