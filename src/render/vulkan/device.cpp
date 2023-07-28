#include "device.hpp"

namespace Vulkan {

const std::vector<vk::Format> valid_formats = {
	vk::Format::eR8G8B8Srgb, vk::Format::eB8G8R8Srgb, vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb,
	vk::Format::eA8B8G8R8SrgbPack32};

struct Config {
	vk::PhysicalDevice physical_device;
	struct Queue {
		u32 family, index;
		f32 priority = 1.0;
	};
	Queue graphics;
	Queue transfer;
	Queue present;
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
			std::vector<vk::QueueFamilyProperties> queue_families = pd.getQueueFamilyProperties();
			int graphics_score = -1;
			for (u32 i = 0; i < queue_families.size(); i++) {
				bool graphics = static_cast<bool>(queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics);
				bool compute = static_cast<bool>(queue_families[i].queueFlags & vk::QueueFlagBits::eCompute);
				bool present = pd.getSurfaceSupportKHR(i, context.surface);
				if (!graphics)
					continue;
				if (!compute)
					continue;

				int score = 10 - present;
				if (score > graphics_score) {
					graphics_score = score;
					config.graphics.family = i;
					config.graphics.index = 0;
				}
			}
			if (graphics_score < 0)
				continue;

			int transfer_score = -1;
			for (u32 i = 0; i < queue_families.size(); i++) {
				bool graphics = static_cast<bool>(queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics);
				bool compute = static_cast<bool>(queue_families[i].queueFlags & vk::QueueFlagBits::eCompute);
				bool transfer = static_cast<bool>(queue_families[i].queueFlags & vk::QueueFlagBits::eTransfer);
				bool present = pd.getSurfaceSupportKHR(i, context.surface);
				if (!transfer)
					continue;

				u32 index = 0;
				int score = 10 - graphics - compute - present;
				if (i == config.graphics.family) {
					index++;
					score--;
				}
				if (index >= queue_families[i].queueCount)
					continue;
				if (score > transfer_score) {
					transfer_score = score;
					config.transfer.family = i;
					config.transfer.index = index;
				}
			}
			if (transfer_score < 0)
				continue;

			int present_score = -1;
			for (u32 i = 0; i < queue_families.size(); i++) {
				bool graphics = static_cast<bool>(queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics);
				bool compute = static_cast<bool>(queue_families[i].queueFlags & vk::QueueFlagBits::eCompute);
				bool transfer = static_cast<bool>(queue_families[i].queueFlags & vk::QueueFlagBits::eTransfer);
				bool present = pd.getSurfaceSupportKHR(i, context.surface);
				if (!present)
					continue;

				u32 index = 0;
				int score = 10 - graphics - compute - transfer;
				if (i == config.graphics.family) {
					index++;
					score--;
				}
				if (i == config.transfer.family) {
					index++;
					score--;
				}
				if (index >= queue_families[i].queueCount)
					continue;
				if (score > present_score) {
					present_score = score;
					config.present.family = i;
					config.present.index = index;
					config.present.priority = 0.5;
				}
			}
			if (present_score < 0)
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
		std::vector<vk::DeviceQueueCreateInfo> queue_create;
		std::vector<std::vector<f32>> priorities;
		std::vector<Config::Queue> queue_configs = {config.graphics, config.transfer, config.present};
		for (auto q : queue_configs) {
			if (priorities.size() <= q.family) {
				priorities.resize(q.family + 1);
			}
			assert(priorities[q.family].size() == q.index);
			priorities[q.family].push_back(q.priority);
		}
		for (u32 i = 0; i < priorities.size(); i++) {
			if (priorities[i].empty())
				continue;
			queue_create.push_back(vk::DeviceQueueCreateInfo({}, i, priorities[i]));
		}

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

	graphics_queue.family = config.graphics.family;
	graphics_queue.queue = device.getQueue(config.graphics.family, config.graphics.index);
	transfer_queue.family = config.transfer.family;
	transfer_queue.queue = device.getQueue(config.transfer.family, config.transfer.index);
	present_queue.family = config.present.family;
	present_queue.queue = device.getQueue(config.present.family, config.present.index);

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
