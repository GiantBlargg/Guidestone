#pragma once

#include "device.hpp"
#include <vulkan/vulkan.hpp>

namespace Vulkan {

class Command {
	vk::Device device;
	vk::Queue queue;

  public:
	size_t index = -1;
	static constexpr size_t size = 3;
	inline size_t get_index() { return index % size; }

  private:
	struct Instance {
		vk::CommandPool pool;
		vk::CommandBuffer cmd;
		vk::Fence fence;
	};

	std::array<Instance, size> instances;
	inline Instance& get_active() { return instances[get_index()]; }

  public:
	operator vk::CommandBuffer() { return get_active().cmd; }
	vk::CommandBuffer* operator->() { return &get_active().cmd; }

	std::vector<vk::SemaphoreSubmitInfo> wait_semaphores;
	std::vector<vk::SemaphoreSubmitInfo> signal_semaphores;

	Command(vk::Device d, const Queue& q);
	~Command();

	void begin();
	void submit();
};

} // namespace Vulkan
