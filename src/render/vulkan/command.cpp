#include "command.hpp"

namespace Vulkan {

RenderCommand::RenderCommand(vk::Device d, const Queue& q) : device(d), queue(q.queue) {
	for (auto& i : instances) {
		i.pool = device.createCommandPool(vk::CommandPoolCreateInfo({}, q.family));
		i.cmd =
			device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(i.pool, vk::CommandBufferLevel::ePrimary, 1))
				.front();
		i.fence = device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
		i.acquire_semaphore = device.createSemaphore({});
		i.rendering_semaphore = device.createSemaphore({});
	}
};
RenderCommand::~RenderCommand() {
	for (auto& i : instances) {
		device.destroyCommandPool(i.pool);
		device.destroyFence(i.fence);
		device.destroySemaphore(i.acquire_semaphore);
		device.destroySemaphore(i.rendering_semaphore);
	}
}

RenderCommand::Instance& RenderCommand::get() {
	auto& i = instances[++index % size];
	auto fence_result = device.waitForFences(i.fence, false, UINT64_MAX);
	if (fence_result != vk::Result::eSuccess) {
		throw new vk::LogicError(to_string(fence_result));
	}
	device.resetFences(i.fence);
	device.resetCommandPool(i.pool);
	i.cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	return i;
};
vk::Fence RenderCommand::submit(Instance& i) {
	i.cmd.end();
	vk::SemaphoreSubmitInfo acquire_semaphore_info(
		i.acquire_semaphore, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	vk::CommandBufferSubmitInfo cmd_info(i.cmd);
	vk::SemaphoreSubmitInfo rendering_semaphore_info(
		i.rendering_semaphore, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	queue.submit2(vk::SubmitInfo2({}, acquire_semaphore_info, cmd_info, rendering_semaphore_info), i.fence);
	return i.fence;
}

} // namespace Vulkan
