#include "vulkan_render.hpp"

#include "engine/log.hpp"

namespace Vulkan {

const std::vector<vk::Format> valid_formats = {
	vk::Format::eR8G8B8Srgb,         vk::Format::eB8G8R8Srgb,         vk::Format::eR8G8B8A8Srgb,
	vk::Format::eB8G8R8A8Srgb,       vk::Format::eA8B8G8R8SrgbPack32, vk::Format::eR16G16B16Sfloat,
	vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32B32Sfloat,    vk::Format::eR32G32B32A32Sfloat,
	vk::Format::eR64G64B64Sfloat,    vk::Format::eR64G64B64A64Sfloat, vk::Format::eB10G11R11UfloatPack32,
};

VulkanRender::VulkanRender(Context::Create c) : context(c) {

	{
		// TODO: Allow changing device
		std::vector<vk::PhysicalDevice> physical_devices = context.instance.enumeratePhysicalDevices();
		std::vector<DeviceConfig> configs;
		for (auto pd : physical_devices) {
			if (pd.getProperties().apiVersion < VK_API_VERSION_1_3)
				continue;

			DeviceConfig config;
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
				auto features = pd.getFeatures2<
					vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
					vk::PhysicalDeviceMemoryPriorityFeaturesEXT>();
				auto features13 = features.get<vk::PhysicalDeviceVulkan13Features>();
				if (!features13.dynamicRendering || !features13.synchronization2)
					continue;
				config.memory_priority |= features.get<vk::PhysicalDeviceMemoryPriorityFeaturesEXT>().memoryPriority;
			}

			{ // Pick a format
				std::vector<vk::SurfaceFormatKHR> supported_formats = pd.getSurfaceFormatsKHR(context.surface);
				supported_formats.erase(
					std::remove_if(
						supported_formats.begin(), supported_formats.end(),
						[](vk::SurfaceFormatKHR format) {
							bool valid_space = format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
							bool valid_format = std::find(valid_formats.begin(), valid_formats.end(), format.format) !=
								valid_formats.end();
							return !(valid_space && valid_format);
						}),
					supported_formats.end());
				if (supported_formats.empty())
					continue;
				config.surface_colour_format = supported_formats.front();
			}

			{ // Pick a queue
				bool found = false;
				std::vector<vk::QueueFamilyProperties> queue_families = pd.getQueueFamilyProperties();
				for (int i = 0; i < queue_families.size(); i++) {
					if (!pd.getSurfaceSupportKHR(i, context.surface))
						continue;

					if (!(queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics))
						continue;

					found = true;
					config.queue_family = i;
				}
				if (!found)
					continue;
			}

			{ // Pick Present Mode
				std::vector<vk::PresentModeKHR> modes = pd.getSurfacePresentModesKHR(context.surface);
				bool mailbox, relaxed;
				for (auto mode : modes) {
					mailbox |= mode == vk::PresentModeKHR::eMailbox;
					relaxed |= mode == vk::PresentModeKHR::eFifoRelaxed;
				}
				config.present_mode = mailbox ? vk::PresentModeKHR::eMailbox
					: relaxed                 ? vk::PresentModeKHR::eFifoRelaxed
											  : vk::PresentModeKHR::eFifo;
				// Force Fifo for early development
				config.present_mode = vk::PresentModeKHR::eFifo;
			}

			configs.push_back(config);
		}
		device_config = configs.front();
	}
	{
		float queue_priority = 1.0f;
		vk::DeviceQueueCreateInfo queue_create({}, device_config.queue_family);
		queue_create.setQueuePriorities(queue_priority);
		std::vector<const char*> device_ext = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
		if (device_config.memory_budget) {
			device_ext.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
		}
		if (device_config.memory_priority) {
			device_ext.push_back(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
		}
		vk::StructureChain<
			vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceMemoryPriorityFeaturesEXT>
			device_info(
				vk::DeviceCreateInfo({}, queue_create, {}, device_ext), vk::PhysicalDeviceVulkan13Features(),
				vk::PhysicalDeviceMemoryPriorityFeaturesEXT(device_config.memory_priority));
		device_info.get<vk::PhysicalDeviceVulkan13Features>().setDynamicRendering(true).setSynchronization2(true);
		device = device_config.physical_device.createDevice(device_info.get());
		VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
	}

	queue = device.getQueue(device_config.queue_family, 0);

	{
		vma::AllocatorCreateInfo alloc_info({}, device_config.physical_device, device);
		if (device_config.memory_budget)
			alloc_info.flags |= vma::AllocatorCreateFlagBits::eExtMemoryBudget;
		if (device_config.memory_priority)
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

	reconfigureSwapchain();

	for (int i = 0; i < frame_concurrency; i++) {
		PerFrame& f = per_frame[i];
		f.command_pool = device.createCommandPool(vk::CommandPoolCreateInfo({}, device_config.queue_family));
		f.command_buffer = device
							   .allocateCommandBuffers(
								   vk::CommandBufferAllocateInfo(f.command_pool, vk::CommandBufferLevel::ePrimary, 1))
							   .front();
		f.submission_fence = device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
		f.acquire_semaphore = device.createSemaphore({});
		f.rendering_semaphore = device.createSemaphore({});
	}
}

VulkanRender::~VulkanRender() {
	device.waitIdle();

	for (int i = 0; i < frame_concurrency; i++) {
		PerFrame& f = per_frame[i];
		device.destroyFence(f.submission_fence);
		device.destroySemaphore(f.acquire_semaphore);
		device.destroySemaphore(f.rendering_semaphore);
		device.destroyCommandPool(f.command_pool);
	}

	for (auto& image_view : image_views)
		device.destroyImageView(image_view);

	device.destroyImageView(depth_view);
	depth_buffer.destroy(allocator);
	allocator.destroy();

	device.destroySwapchainKHR(swapchain);
	device.destroy();
}

void VulkanRender::reconfigureSwapchain() {
	update_swapchain = false;

	// Seems to work without
	device.waitIdle();

	vk::SurfaceCapabilitiesKHR caps = device_config.physical_device.getSurfaceCapabilitiesKHR(context.surface);
	if (caps.currentExtent != vk::Extent2D(0xFFFFFFFF, 0xFFFFFFFF) && caps.currentExtent != vk::Extent2D(0, 0)) {
		surface_extent = caps.currentExtent;
	} else {
		if (surface_extent.width < caps.minImageExtent.width)
			surface_extent.width = caps.minImageExtent.width;
		if (surface_extent.height < caps.minImageExtent.height)
			surface_extent.height = caps.minImageExtent.height;
		if (surface_extent.width > caps.maxImageExtent.width)
			surface_extent.width = caps.maxImageExtent.width;
		if (surface_extent.height > caps.maxImageExtent.height)
			surface_extent.height = caps.maxImageExtent.height;
	}

	uint32_t image_count = 3;
	if (image_count < caps.minImageCount)
		image_count = caps.minImageCount;
	if (caps.maxImageCount != 0 && image_count > caps.maxImageCount)
		image_count = caps.maxImageCount;

	{
		vk::SwapchainKHR old_swapchain = swapchain;

		swapchain = device.createSwapchainKHR(vk::SwapchainCreateInfoKHR(
			{}, context.surface, image_count, device_config.surface_colour_format.format,
			device_config.surface_colour_format.colorSpace, surface_extent, 1, vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive, {}, vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true, old_swapchain));

		if (old_swapchain)
			device.destroySwapchainKHR(old_swapchain);
	}
	images = device.getSwapchainImagesKHR(swapchain);

	for (auto& image_view : image_views)
		device.destroyImageView(image_view);
	image_views.resize(images.size());
	for (size_t i = 0; i < image_views.size(); i++) {
		image_views[i] = device.createImageView(vk::ImageViewCreateInfo(
			{}, images[i], vk::ImageViewType::e2D, device_config.surface_colour_format.format, {},
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
	}

	if (depth_view) {
		device.destroyImageView(depth_view);
		depth_buffer.destroy(allocator);
	}

	{
		vk::Format depth_format = vk::Format::eD32Sfloat;
		vk::ImageCreateInfo depth_info(
			{}, vk::ImageType::e2D, depth_format, vk::Extent3D(surface_extent.width, surface_extent.height, 1), 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::SharingMode::eExclusive, device_config.queue_family, vk::ImageLayout::eUndefined);
		vma::AllocationCreateInfo alloc_info;
		alloc_info.setUsage(vma::MemoryUsage::eGpuOnly).setPriority(1);
		depth_buffer = allocator.createImage(depth_info, alloc_info);
		depth_view = device.createImageView(vk::ImageViewCreateInfo(
			{}, depth_buffer, vk::ImageViewType::e2D, depth_format, {},
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)));
	}
}

void VulkanRender::resize(uint32_t width, uint32_t height) {
	vk::Extent2D new_extent{width, height};
	if (surface_extent == new_extent)
		return;

	update_swapchain = true;
	surface_extent = new_extent;
}

uint32_t VulkanRender::acquireImage(vk::Semaphore semaphore) {
	if (update_swapchain)
		reconfigureSwapchain();

	auto image_index_result = device.acquireNextImageKHR(swapchain, UINT64_MAX, semaphore, nullptr);
	if (image_index_result.result != vk::Result::eSuccess) {
		update_swapchain = true;
		if (image_index_result.result != vk::Result::eSuboptimalKHR)
			return acquireImage(semaphore);
	}
	return image_index_result.value;
}

void VulkanRender::renderFrame() {
	frame_index++;
	auto& frame = per_frame[frame_index % frame_concurrency];

	auto fence_result = device.waitForFences(frame.submission_fence, false, UINT64_MAX);
	if (fence_result != vk::Result::eSuccess) {
		throw new vk::LogicError(to_string(fence_result));
	}
	device.resetFences(frame.submission_fence);
	device.resetCommandPool(frame.command_pool);

	uint32_t image_index = acquireImage(frame.acquire_semaphore);

	{
		frame.command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		{
			std::vector<vk::ImageMemoryBarrier2> image_barrier;
			image_barrier.push_back(vk::ImageMemoryBarrier2(
				vk::PipelineStageFlagBits2::eColorAttachmentOutput, {},
				vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, device_config.queue_family,
				device_config.queue_family, images[image_index],
				vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
			image_barrier.push_back(vk::ImageMemoryBarrier2(
				{}, {}, vk::PipelineStageFlagBits2::eEarlyFragmentTests,
				vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::ImageLayout::eUndefined,
				vk::ImageLayout::eDepthAttachmentOptimal, device_config.queue_family, device_config.queue_family,
				depth_buffer, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)));

			frame.command_buffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
		}

		{
			vk::Viewport viewport(0, 0, surface_extent.width, surface_extent.height, 0, 1);
			frame.command_buffer.setViewport(0, viewport);
			vk::Rect2D scissors({0, 0}, surface_extent);
			frame.command_buffer.setScissor(0, scissors);

			vk::RenderingAttachmentInfo colour_attachment(
				image_views[image_index], vk::ImageLayout::eColorAttachmentOptimal);
			colour_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
#if 1
			vk::ClearValue clear_value(vk::ClearColorValue(std::array<float, 4>({{0.0, 0.3, 0.8, 1.0}})));
			colour_attachment.setLoadOp(vk::AttachmentLoadOp::eClear).setClearValue(clear_value);
#else
			colour_attachment.setLoadOp(vk::AttachmentLoadOp::eDontCare);
#endif
			vk::RenderingAttachmentInfo depth_attachment(depth_view, vk::ImageLayout::eDepthAttachmentOptimal);
			depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setClearValue(vk::ClearValue(vk::ClearDepthStencilValue(0)));
			vk::RenderingInfo rendering_info({}, scissors, 1, 0, colour_attachment);
			frame.command_buffer.beginRendering(rendering_info);
		}

		frame.command_buffer.endRendering();

		{
			vk::ImageMemoryBarrier2 image_barrier(
				vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
				vk::PipelineStageFlagBits2::eColorAttachmentOutput, {}, vk::ImageLayout::eColorAttachmentOptimal,
				vk::ImageLayout::ePresentSrcKHR, device_config.queue_family, device_config.queue_family,
				images[image_index], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
			frame.command_buffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
		}

		frame.command_buffer.end();
	}

	vk::SemaphoreSubmitInfo acquire_semaphore_info(
		frame.acquire_semaphore, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	vk::CommandBufferSubmitInfo cmd_info(frame.command_buffer);
	vk::SemaphoreSubmitInfo rendering_semaphore_info(
		frame.rendering_semaphore, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	vk::SubmitInfo2 submit_info({}, acquire_semaphore_info, cmd_info, rendering_semaphore_info);
	queue.submit2(submit_info, frame.submission_fence);

	vk::Result result = queue.presentKHR(vk::PresentInfoKHR(frame.rendering_semaphore, swapchain, image_index));
	if (result != vk::Result::eSuccess)
		update_swapchain = true;
}

} // namespace Vulkan
