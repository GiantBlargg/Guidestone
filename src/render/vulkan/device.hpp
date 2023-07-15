#pragma once

#include "context.hpp"
#include "engine/types.hpp"
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

namespace Vulkan {
struct Queue {
	vk::Queue queue;
	u32 family;
};

struct Device {
	vk::PhysicalDevice physical_device;
	vk::SurfaceFormatKHR surface_format;
	vk::PresentModeKHR present_mode;
	vk::Format depth_format;

	vk::Device device;
	Queue graphics_queue;

	vma::Allocator allocator;

	Device(Device&) = delete;
	Device(const Context&);
	~Device();

	operator vk::Device() const { return device; }
	const vk::Device* operator->() const { return &device; }
};

} // namespace Vulkan
