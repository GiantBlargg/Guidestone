#pragma once

#include "device.hpp"
#include <vulkan/vulkan.hpp>

namespace Vulkan {

class RenderCommand {
	vk::Device device;
	vk::Queue queue;

  public:
	static constexpr size_t size = 3;

	struct Instance {
		vk::CommandPool pool;
		vk::CommandBuffer cmd;
		vk::Fence fence;

		vk::Semaphore acquire_semaphore;
		vk::Semaphore rendering_semaphore;

		operator vk::CommandBuffer() { return cmd; }
		vk::CommandBuffer* operator->() { return &cmd; }
	};

	Instance instances[size];
	size_t index = -1;
	size_t get_index() { return index % size; }

	RenderCommand(vk::Device d, const Queue& q);
	~RenderCommand();

	Instance& get();
	vk::Fence submit(Instance& i);
};

} // namespace Vulkan
