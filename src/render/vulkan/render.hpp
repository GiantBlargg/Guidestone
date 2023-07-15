#pragma once

#include "command.hpp"
#include "context.hpp"
#include "device.hpp"
#include "engine/render.hpp"
#include "swapchain.hpp"
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Vulkan {

struct ImageAllocation {
	vk::Image image;
	vma::Allocation alloc;

	ImageAllocation() = default;
	ImageAllocation(const std::pair<vk::Image, vma::Allocation>& p) : image(p.first), alloc(p.second) {}

	ImageAllocation& operator=(const std::pair<vk::Image, vma::Allocation>& p) {
		image = p.first;
		alloc = p.second;
		return *this;
	}

	explicit operator bool() { return image; }

	operator vk::Image() { return image; }
	operator vma::Allocation() { return alloc; }

	void destroy(vma::Allocator allocator) { return allocator.destroyImage(image, alloc); }
};

struct BufferAllocation {
	vk::Buffer buffer;
	vma::Allocation alloc;

	BufferAllocation() = default;
	BufferAllocation(const std::pair<vk::Buffer, vma::Allocation>& p) : buffer(p.first), alloc(p.second) {}

	BufferAllocation& operator=(const std::pair<vk::Buffer, vma::Allocation>& p) {
		buffer = p.first;
		alloc = p.second;
		return *this;
	}

	explicit operator bool() { return buffer; }

	operator vk::Buffer() { return buffer; }
	operator vma::Allocation() { return alloc; }

	void destroy(vma::Allocator allocator) { return allocator.destroyBuffer(buffer, alloc); }
};

class Render : public ::Render {
	const Context context;
	const Device device;
	Swapchain swapchain;

	struct PerFrame {
		vk::Semaphore acquire_semaphore;
		vk::Semaphore rendering_semaphore;
	};
	Command<3, PerFrame> render_cmd;

	vk::Extent2D frame_extent;
	ImageAllocation depth_buffer;
	vk::ImageView depth_view;
	void resize_frame();

	BufferAllocation vertex_buffer;
	vk::Pipeline default_pipeline;
	vk::PipelineLayout layout;

  public:
	Render(Context::Create);
	~Render();

	void resize(uint32_t width, uint32_t height) override { swapchain.set_extent(vk::Extent2D{width, height}); }
	void renderFrame() override;
	void setModelCache(const ModelCache&) override;
};

} // namespace Vulkan
