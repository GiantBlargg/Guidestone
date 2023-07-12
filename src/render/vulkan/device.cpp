#include "device.hpp"

namespace Vulkan {

const std::vector<vk::Format> valid_formats = {
	vk::Format::eR8G8B8Srgb,         vk::Format::eB8G8R8Srgb,         vk::Format::eR8G8B8A8Srgb,
	vk::Format::eB8G8R8A8Srgb,       vk::Format::eA8B8G8R8SrgbPack32, vk::Format::eR16G16B16Sfloat,
	vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32B32Sfloat,    vk::Format::eR32G32B32A32Sfloat,
	vk::Format::eR64G64B64Sfloat,    vk::Format::eR64G64B64A64Sfloat, vk::Format::eB10G11R11UfloatPack32,
};

struct Config {
	vk::PhysicalDevice physical_device;
	uint32_t graphics_family;
	uint32_t transfer_family;
	uint32_t present_family;
	vk::SurfaceFormatKHR surface_format;
	vk::PresentModeKHR present_mode;
	vk::Format depth_format;
	bool memory_budget;
	bool memory_priority;
};

std::vector<Config> getConfigs(const Context& context) {
	std::vector<vk::PhysicalDevice> physical_devices = context.instance.enumeratePhysicalDevices();
	std::vector<Config> configs;
	configs.reserve(physical_devices.size());

	for (auto pd : physical_devices) {
		if (pd.getProperties().apiVersion < VK_API_VERSION_1_3)
			continue;

		Config config;
		config.physical_device = pd;

		{
			for (auto ext : pd.enumerateDeviceExtensionProperties()) {
				if (!strcmp(ext.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)) {
					config.memory_budget = true;
				}
				if (!strcmp(ext.extensionName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME)) {
					config.memory_priority = true;
				}
			}
		}

		{
			vk::StructureChain<
				vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
				vk::PhysicalDeviceMemoryPriorityFeaturesEXT>
				features;
			if (!config.memory_priority)
				features.unlink<vk::PhysicalDeviceMemoryPriorityFeaturesEXT>();
			pd.getFeatures2(&features.get());
			auto features13 = features.get<vk::PhysicalDeviceVulkan13Features>();
			if (!features13.dynamicRendering || !features13.synchronization2)
				continue;
			if (config.memory_budget)
				config.memory_priority = features.get<vk::PhysicalDeviceMemoryPriorityFeaturesEXT>().memoryPriority;
		}

		{ // Pick surface format
			std::vector<vk::SurfaceFormatKHR> supported_formats = pd.getSurfaceFormatsKHR(context.surface);
			supported_formats.erase(
				std::remove_if(
					supported_formats.begin(), supported_formats.end(),
					[](vk::SurfaceFormatKHR format) {
						bool valid_space = format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
						bool valid_format =
							std::find(valid_formats.begin(), valid_formats.end(), format.format) != valid_formats.end();
						return !(valid_space && valid_format);
					}),
				supported_formats.end());
			if (supported_formats.empty())
				continue;
			config.surface_format = supported_formats.front();
		}

		{ // Pick queues
			bool found = false;
			std::vector<vk::QueueFamilyProperties> queue_families = pd.getQueueFamilyProperties();
			for (int i = 0; i < queue_families.size(); i++) {
				if (!pd.getSurfaceSupportKHR(i, context.surface))
					continue;

				if (!(queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics))
					continue;

				found = true;
				config.graphics_family = i;
			}
			if (!found)
				continue;
		}

		{ // Pick Present Mode
			std::vector<vk::PresentModeKHR> modes = pd.getSurfacePresentModesKHR(context.surface);
			bool mailbox = false, relaxed = false;
			for (auto mode : modes) {
				// Force Fifo for early development
				// mailbox = mailbox || (mode == vk::PresentModeKHR::eMailbox);
				relaxed = relaxed || (mode == vk::PresentModeKHR::eFifoRelaxed);
			}
			config.present_mode = mailbox ? vk::PresentModeKHR::eMailbox
				: relaxed                 ? vk::PresentModeKHR::eFifoRelaxed
										  : vk::PresentModeKHR::eFifo;
		}

		{ // Pick Depth Format
			bool depth32 = static_cast<bool>(
				pd.getFormatProperties(vk::Format::eD32Sfloat).optimalTilingFeatures &
				vk::FormatFeatureFlagBits::eDepthStencilAttachment);

			config.depth_format = depth32 ? vk::Format::eD32Sfloat : vk::Format::eX8D24UnormPack32;
		}

		configs.push_back(config);
	}
	return configs;
}

Device::Device(const Context& context) {
	auto configs = getConfigs(context);
	// TODO: Load choice from options
	auto config = configs.front();
	{
		float queue_priority = 1.0f;
		vk::DeviceQueueCreateInfo queue_create({}, config.graphics_family);
		queue_create.setQueuePriorities(queue_priority);
		std::vector<const char*> device_ext = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
		if (config.memory_budget) {
			device_ext.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
		}
		if (config.memory_priority) {
			device_ext.push_back(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
		}
		vk::StructureChain<
			vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceMemoryPriorityFeaturesEXT>
			device_info(
				vk::DeviceCreateInfo({}, queue_create, {}, device_ext), vk::PhysicalDeviceVulkan13Features(),
				vk::PhysicalDeviceMemoryPriorityFeaturesEXT(true));
		device_info.get<vk::PhysicalDeviceVulkan13Features>().setDynamicRendering(true).setSynchronization2(true);
		if (!config.memory_priority)
			device_info.unlink<vk::PhysicalDeviceMemoryPriorityFeaturesEXT>();
		device = config.physical_device.createDevice(device_info.get());
		VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
	}

	physical_device = config.physical_device;
	surface_format = config.surface_format;
	present_mode = config.present_mode;
	depth_format = config.depth_format;

	graphics_queue.family = config.graphics_family;
	graphics_queue.queue = device.getQueue(graphics_queue.family, 0);

	{
		vma::AllocatorCreateInfo alloc_info({}, config.physical_device, device);
		if (config.memory_budget)
			alloc_info.flags |= vma::AllocatorCreateFlagBits::eExtMemoryBudget;
		if (config.memory_priority)
			alloc_info.flags |= vma::AllocatorCreateFlagBits::eExtMemoryPriority;

		vma::VulkanFunctions vkfuncs{
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceBufferMemoryRequirements,
			VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceImageMemoryRequirements,
		};
		alloc_info.setPVulkanFunctions(&vkfuncs);
		alloc_info.setInstance(context.instance);
		alloc_info.setVulkanApiVersion(VK_API_VERSION_1_3);
		allocator = vma::createAllocator(alloc_info);
	}
}
Device::~Device() {
	allocator.destroy();
	device.destroy();
}

} // namespace Vulkan
