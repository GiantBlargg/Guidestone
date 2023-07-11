#pragma once

#include "context.hpp"
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

namespace Vulkan {

struct Device {
	struct Config {
		vk::PhysicalDevice physical_device;
		uint32_t queue_family;
		vk::SurfaceFormatKHR surface_colour_format;
		vk::PresentModeKHR present_mode;
		bool memory_budget;
		bool memory_priority;
	} config;
	vk::Device device;
	vk::Queue queue;

	vma::Allocator allocator;

	Device(Device&) = delete;
	Device(const Context&);
	~Device();

	operator vk::Device() const { return device; }
};

} // namespace Vulkan
