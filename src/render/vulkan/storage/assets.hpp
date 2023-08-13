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

  public:
	BufferAllocation vertex;
	std::vector<ImageAllocation> textures;

	Assets(const Device&);
	~Assets();

	void set_model_cache(const ModelCache&);
};

} // namespace Vulkan
