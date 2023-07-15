#pragma once

#include "device.hpp"
#include <vulkan/vulkan.hpp>

namespace Vulkan {

struct Empty {};

template <size_t N, class T = Empty> class Command {
	vk::Device device;
	const Queue& queue;

  public:
	const size_t size = N;

	struct Instance {
		vk::CommandPool pool;
		vk::CommandBuffer buffer;
		vk::Fence fence;

		T data;

		vk::CommandBuffer* operator->() { return &buffer; }
	};

  private:
	Instance instances[N];
	size_t index = -1;

  public:
	Command(vk::Device d, const Queue& q) : device(d), queue(q) {
		for (auto& i : instances) {
			i.pool = device.createCommandPool(vk::CommandPoolCreateInfo({}, q.family));
			i.buffer =
				device
					.allocateCommandBuffers(vk::CommandBufferAllocateInfo(i.pool, vk::CommandBufferLevel::ePrimary, 1))
					.front();
			i.fence = device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
		}
	};
	~Command() {
		for (auto& i : instances) {
			device.destroyCommandPool(i.pool);
			device.destroyFence(i.fence);
		}
	}

	void forEach(std::function<void(T&)> f) {
		for (auto& i : instances) {
			f(i.data);
		}
	}

	Instance& get() {
		auto& i = instances[++index % N];
		auto fence_result = device.waitForFences(i.fence, false, UINT64_MAX);
		if (fence_result != vk::Result::eSuccess) {
			throw new vk::LogicError(to_string(fence_result));
		}
		device.resetFences(i.fence);
		device.resetCommandPool(i.pool);
		i.buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		return i;
	};
	vk::Fence submit(
		Instance& i, vk::ArrayProxyNoTemporaries<const vk::SemaphoreSubmitInfo> const& waitSemaphoreInfos,
		vk::ArrayProxyNoTemporaries<const vk::SemaphoreSubmitInfo> const& signalSemaphoreInfos) {
		i.buffer.end();
		vk::CommandBufferSubmitInfo cmd_info(i.buffer);
		queue.queue.submit2(vk::SubmitInfo2({}, waitSemaphoreInfos, cmd_info, signalSemaphoreInfos), i.fence);
		return i.fence;
	}
};

} // namespace Vulkan
