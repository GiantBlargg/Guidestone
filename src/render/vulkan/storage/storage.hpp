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

	void init(const Device&, const vk::ImageCreateInfo&, const vma::AllocationCreateInfo&);
	void init(const Device&, const vk::ImageCreateInfo&, const vma::AllocationCreateInfo&, vk::ImageViewCreateInfo&);

	void destroy(const Device&);
};

struct BufferAllocation {
	vk::Buffer buffer;
	vma::Allocation alloc;
	void* ptr;

	explicit operator bool() { return buffer; }

	operator vk::Buffer() { return buffer; }
	operator vma::Allocation() { return alloc; }
	operator void*() { return ptr; }

	void init(const Device&, const vk::BufferCreateInfo&, const vma::AllocationCreateInfo&);

	void destroy(const Device&);
};

struct Storage {
  private:
	const Device& device;

  public:
	BufferAllocation vertex_buffer;
	void update_vertex_buffer(const void* src, size_t n);

	Storage(const Device&);
	~Storage();

	void bind_buffers(vk::CommandBuffer, size_t index);
};

} // namespace Vulkan
