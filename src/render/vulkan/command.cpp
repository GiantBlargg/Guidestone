#include "command.hpp"

namespace Vulkan {

Command::Command(vk::Device d, const Queue& q) : device(d), queue(q.queue) {
	for (auto& i : instances) {
		i.pool = device.createCommandPool(vk::CommandPoolCreateInfo({}, q.family));
		i.cmd =
			device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(i.pool, vk::CommandBufferLevel::ePrimary, 1))
				.front();
		i.fence = device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}
};
Command::~Command() {
	for (auto& i : instances) {
		device.destroyCommandPool(i.pool);
		device.destroyFence(i.fence);
	}
}

void Command::begin() {
	auto& i = instances[++index % size];
	auto fence_result = device.waitForFences(i.fence, false, UINT64_MAX);
	if (fence_result != vk::Result::eSuccess) {
		throw new vk::LogicError(to_string(fence_result));
	}
	device.resetFences(i.fence);
	device.resetCommandPool(i.pool);
	i.cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
};

void Command::submit() {
	auto& i = get_active();
	i.cmd.end();
	vk::CommandBufferSubmitInfo cmd_info(i.cmd);
	queue.submit2(vk::SubmitInfo2({}, wait_semaphores, cmd_info, signal_semaphores), i.fence);
	wait_semaphores.clear();
	signal_semaphores.clear();
}

} // namespace Vulkan
