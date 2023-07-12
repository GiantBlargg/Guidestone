#pragma once

#include "context.hpp"
#include "engine/types.hpp"
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

namespace Vulkan {
struct Queue {
	vk::Queue queue;
	u32 family;

	void submit(vk::ArrayProxy<const vk::SubmitInfo2> const& submits, vk::Fence fence = {}) const {
		return queue.submit2(submits, fence);
	}
};

struct PresentQueue : public Queue {
	[[nodiscard]] vk::Result present(const vk::PresentInfoKHR& presentInfo) const {
		return queue.presentKHR(presentInfo);
	}
};

struct Device {
	vk::PhysicalDevice physical_device;
	vk::SurfaceFormatKHR surface_format;
	vk::PresentModeKHR present_mode;
	vk::Format depth_format;

	vk::Device device;
	PresentQueue graphics_queue;

	vma::Allocator allocator;

	Device(Device&) = delete;
	Device(const Context&);
	~Device();

	operator vk::Device() const { return device; }
};

} // namespace Vulkan
