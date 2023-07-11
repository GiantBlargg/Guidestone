#pragma once

#include <functional>
#include <vulkan/vulkan.hpp>

namespace Vulkan {

struct Context {
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debug_messenger;
	vk::SurfaceKHR surface;

	Context(Context&) = delete;
	struct Create {
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
		std::vector<const char*> extensions;
		std::function<void(VkInstance, VkSurfaceKHR*)> make_surface;
	};
	Context(Create);
	~Context();
};

} // namespace Vulkan
