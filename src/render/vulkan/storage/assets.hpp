#pragma once

#include "command.hpp"
#include "device.hpp"
#include "model.hpp"
#include "storage.hpp"
#include <atomic>

namespace Vulkan {

class Assets {
	const Device& device;
	Command cmd;

	vk::DescriptorPool desc_pool;

  public:
	BufferAllocation vertex;
	std::vector<ImageAllocation> textures;

	vk::Sampler sampler;
	vk::DescriptorSetLayout material_layout;
	std::vector<vk::DescriptorSet> material_sets;

	Assets(const Device&);
	~Assets();

	void set_model_cache(const ModelCache&);
};

} // namespace Vulkan
