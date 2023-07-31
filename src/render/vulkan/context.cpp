#include "context.hpp"

#include "log.hpp"

namespace Vulkan {

VkBool32 debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
	switch (static_cast<uint32_t>(pCallbackData->messageIdNumber)) {

	// UNASSIGNED-BestPractices-CreateImage-Depth32Format
	// AMD is fine with 32-bit Depth Buffers
	case 0x53bb41ae:

	// UNASSIGNED-BestPractices-ClearColor-NotCompressed
	// Will Eventually switch to DontCare
	case 0x6f7a814b:

	// UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension-debugging
	// Debugging is disabled in Release
	case 0x822806fa:

		return false;
	default:
		break;
	}

	Log::Severity severity;

	switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) {
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
		severity = Log::Severity::Trace;
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
		severity = Log::Severity::Info;
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
		severity = Log::Severity::Warning;
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
		severity = Log::Severity::Error;
		break;
	}

	std::string message_type = "Vulkan " + to_string(static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(messageTypes)) +
		" " + to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity));
	if (pCallbackData->pMessageIdName != nullptr) {
		message_type = message_type + ": " + pCallbackData->pMessageIdName;
	}

	Log::log(Log::Msg{.severity = severity, .type = message_type, .msg = pCallbackData->pMessage});

	return false;
}

vk::DebugUtilsMessengerCreateInfoEXT const debug_messenger_create_info(
	{},
	vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
	vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
	debug_callback);

Context::Context(Create c) {

	VULKAN_HPP_DEFAULT_DISPATCHER.init(c.vkGetInstanceProcAddr);

#ifndef NDEBUG
	bool debug = true;
#else
	bool debug = false;
#endif

	if (debug) {
		bool can_debug = false;
		for (auto& ext : vk::enumerateInstanceExtensionProperties()) {
			if (ext == VkExtensionProperties{VK_EXT_DEBUG_UTILS_EXTENSION_NAME, 2}) {
				can_debug = true;
			}
		}
		if (!can_debug) {
			debug = false;
			Log::warn("Vulkan logging not available");
		}
	}

	if (debug) {
		c.extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	vk::ApplicationInfo app_info("Guidestone", 0, "Guidestone", 0, VK_API_VERSION_1_3);
	vk::InstanceCreateInfo instance_create({}, &app_info, {}, c.extensions);

	if (debug) {
		instance_create.setPNext(&debug_messenger_create_info);
	}

	instance = vk::createInstance(instance_create);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

	if (debug) {
		debug_messenger = instance.createDebugUtilsMessengerEXT(debug_messenger_create_info);
	}

	VkSurfaceKHR s;
	c.make_surface(instance, &s);
	surface = s;
}

Context::~Context() {
	instance.destroy(surface);

	if (debug_messenger)
		instance.destroyDebugUtilsMessengerEXT(debug_messenger);

	instance.destroy();
}

} // namespace Vulkan
