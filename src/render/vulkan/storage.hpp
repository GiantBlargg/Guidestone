#pragma once

#include "device.hpp"
#include "math.hpp"
#include "model.hpp"
#include "swapchain.hpp"
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

namespace Vulkan {

struct ImageAllocation {
	vk::Image image;
	vma::Allocation alloc;
	vk::ImageView view;

	explicit operator bool() { return image; }

	operator vk::Image() { return image; }
	operator vk::ImageView() { return view; }
	operator vma::Allocation() { return alloc; }

	void init(const Device& device, const vk::ImageCreateInfo& image_info, const vma::AllocationCreateInfo& alloc_info);
	void init(
		const Device& device, const vk::ImageCreateInfo& image_info, const vma::AllocationCreateInfo& alloc_info,
		vk::ImageViewCreateInfo& view_info);

	void destroy(const Device& device);
};

struct BufferAllocation {
	vk::Buffer buffer;
	vma::Allocation alloc;
	void* ptr;

	explicit operator bool() { return buffer; }

	operator vk::Buffer() { return buffer; }
	operator vma::Allocation() { return alloc; }
	operator void*() { return ptr; }

	void
	init(const Device& device, const vk::BufferCreateInfo& buffer_info, const vma::AllocationCreateInfo& alloc_info);

	void destroy(const Device& device);
};

struct Uniform {
	Matrix4 camera; // Projection * View
};

struct Storage {
  private:
	const Device& device;

  public:
	vk::Extent2D frame_extent;
	void resize_frame(vk::Extent2D);

	ImageAllocation depth_buffer;

	BufferAllocation vertex_buffer;
	void update_vertex_buffer(const void* src, size_t n);

	vk::PipelineLayout pipeline_layout;

	vk::DescriptorPool uniform_pool;
	vk::DescriptorSetLayout uniform_layout;
	vk::DescriptorSet uniform_set;
	BufferAllocation uniform_buffer;
	vk::DeviceSize uniform_stride;
	void update_uniform(const Uniform&, size_t index);

	Storage(const Device&);
	~Storage();

	void start_render(vk::CommandBuffer, Swapchain::Image, size_t index);
	void end_render(vk::CommandBuffer, Swapchain::Image);
};

} // namespace Vulkan
