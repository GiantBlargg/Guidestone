#include "assets.hpp"

namespace Vulkan {

template <class T> inline size_t vectorSize(std::vector<T> v) { return v.size() * sizeof(T); }

struct Staging {
	struct CopyBase {
		const void* source;
		vk::DeviceSize size;
		vk::DeviceSize offset = 0;
	};
	struct CopyBuffer : CopyBase {
		vk::Buffer dst;
	};
	std::vector<CopyBuffer> copy_buffers;
	struct CopyImage : CopyBase {
		vk::Image dst;
		vk::Extent3D extent;
	};
	std::vector<CopyImage> copy_images;

	BufferAllocation staging_buffer;

	template <typename T> void prepare(const std::vector<T>& data, vk::Buffer buf) {
		copy_buffers.push_back(CopyBuffer{{data.data(), vectorSize(data)}, buf});
	}
	template <typename T> void prepare(const std::vector<T>& data, vk::Image img, vk::Extent3D extent) {
		copy_images.push_back(CopyImage{{data.data(), vectorSize(data)}, img, extent});
	}

	void run(const Device& device, vk::CommandBuffer cmd) {
		vk::DeviceSize staging_size = 0;

		auto pack_copies = [&staging_size](CopyBase& copy) {
			copy.offset = staging_size;
			staging_size += copy.size;
		};
		std::ranges::for_each(copy_buffers, pack_copies);
		std::ranges::for_each(copy_images, pack_copies);

		std::vector<vk::ImageMemoryBarrier2> pre_image_barriers;
		pre_image_barriers.reserve(copy_images.size());
		std::vector<vk::ImageMemoryBarrier2> post_image_barriers;
		post_image_barriers.reserve(copy_images.size());
		for (auto i : copy_images) {
			pre_image_barriers.emplace_back();
			pre_image_barriers.back()
				.setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
				.setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setImage(i.dst)
				.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setLevelCount(1)
				.setLayerCount(1);

			post_image_barriers.emplace_back();
			post_image_barriers.back()
				.setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
				.setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
				.setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
				.setDstAccessMask(vk::AccessFlagBits2::eShaderSampledRead)
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImage(i.dst)
				.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setLevelCount(1)
				.setLayerCount(1);
		}

		cmd.pipelineBarrier2(vk::DependencyInfo{
			{},
			{},
			{},
			pre_image_barriers,
		});

		vk::BufferCreateInfo staging_create({}, staging_size, vk::BufferUsageFlagBits::eTransferSrc);
		vma::AllocationCreateInfo staging_alloc(
			vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
			vma::MemoryUsage::eAutoPreferHost);
		staging_buffer.init(device, staging_create, staging_alloc);

		auto copy_to_staging = [this](CopyBase& copy) {
			memcpy((u8*)(staging_buffer.ptr) + copy.offset, copy.source, copy.size);
		};
		std::ranges::for_each(copy_buffers, copy_to_staging);
		std::ranges::for_each(copy_images, copy_to_staging);

		for (auto& buffer : copy_buffers) {
			vk::BufferCopy region(buffer.offset, 0, buffer.size);
			cmd.copyBuffer(staging_buffer, buffer.dst, region);
		}

		for (auto& image : copy_images) {
			vk::ImageSubresourceLayers sub(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
			vk::BufferImageCopy region(image.offset, 0, 0, sub, {0, 0, 0}, image.extent);
			cmd.copyBufferToImage(staging_buffer, image.dst, vk::ImageLayout::eTransferDstOptimal, region);
		}
		cmd.pipelineBarrier2(vk::DependencyInfo{
			{},
			{},
			{},
			post_image_barriers,
		});
	}
};

Assets::Assets(const Device& d) : device(d), cmd(device, device.graphics_queue) {}
Assets::~Assets() {
	if (vertex)
		vertex.destroy(device);
	for (auto& tex : textures)
		tex.destroy(device);
}

void Assets::set_model_cache(const ModelCache& models) {
	Staging staging;

	vk::BufferCreateInfo vertex_info(
		{}, vectorSize(models.vertices),
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
	vma::AllocationCreateInfo vertex_alloc({}, vma::MemoryUsage::eAutoPreferDevice);
	vertex.init(device, vertex_info, vertex_alloc);
	staging.prepare(models.vertices, vertex);

	textures.resize(models.textures.size());

	for (size_t i = 0; i < models.textures.size(); i++) {
		const ModelCache::Texture& tex_data = models.textures[i];
		ImageAllocation& tex = textures[i];

		vk::Extent3D extent(tex_data.width, tex_data.height, 1);
		vk::ImageCreateInfo tex_create;
		tex_create.setImageType(vk::ImageType::e2D)
			.setFormat(vk::Format::eR8G8B8A8Srgb)
			.setExtent(extent)
			.setMipLevels(1)
			.setArrayLayers(1)
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
		vma::AllocationCreateInfo tex_alloc({}, vma::MemoryUsage::eAutoPreferDevice);
		tex.init(device, tex_create, tex_alloc);
		staging.prepare(tex_data.rgba, tex, extent);
	}

	cmd.begin();
	staging.run(device, cmd);
	cmd.submit();
	device->waitIdle();
	staging.staging_buffer.destroy(device);
}

} // namespace Vulkan
