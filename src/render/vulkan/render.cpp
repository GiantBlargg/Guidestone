#include "render.hpp"

#include "engine/log.hpp"
#include "shaders.hpp"

namespace Vulkan {

template <class T> inline size_t vectorSize(std::vector<T> v) { return v.size() * sizeof(T); }

Render::Render(Context::Create c)
	: context(c), device(context), swapchain(context.surface, device), storage(device),
	  render_cmd(device, device.graphics_queue) {

	{
		vk::ShaderModule vertex_shader =
			device->createShaderModule(vk::ShaderModuleCreateInfo({}, Shaders::default_vert));
		vk::ShaderModule fragment_shader =
			device->createShaderModule(vk::ShaderModuleCreateInfo({}, Shaders::default_frag));
		std::vector<vk::PipelineShaderStageCreateInfo> stages = {
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertex_shader, "main"),
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragment_shader, "main")};

		std::vector<vk::VertexInputBindingDescription> bindings = {
			vk::VertexInputBindingDescription(0, sizeof(ModelCache::Vertex))};
		std::vector<vk::VertexInputAttributeDescription> attrib = {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0)};
		vk::PipelineVertexInputStateCreateInfo vertex_state({}, bindings, attrib);

		vk::PipelineInputAssemblyStateCreateInfo assembly_state({}, vk::PrimitiveTopology::eTriangleList);

		vk::PipelineViewportStateCreateInfo viewport({}, 1, nullptr, 1, nullptr);

		vk::PipelineRasterizationStateCreateInfo raster;
		raster.setLineWidth(1.0);

		vk::PipelineMultisampleStateCreateInfo multisample;

		vk::PipelineDepthStencilStateCreateInfo depth({}, true, true, vk::CompareOp::eGreater);

		vk::PipelineColorBlendAttachmentState blend_attach(false);
		blend_attach.setColorWriteMask(
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA);
		vk::PipelineColorBlendStateCreateInfo blend;
		blend.setAttachments(blend_attach);

		std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);

		vk::PipelineLayoutCreateInfo layout_info;
		layout = device->createPipelineLayout(layout_info);

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipeline_create;
		pipeline_create.get()
			.setStages(stages)
			.setPVertexInputState(&vertex_state)
			.setPInputAssemblyState(&assembly_state)
			.setPViewportState(&viewport)
			.setPRasterizationState(&raster)
			.setPMultisampleState(&multisample)
			.setPDepthStencilState(&depth)
			.setPColorBlendState(&blend)
			.setPDynamicState(&dynamic)
			.setLayout(layout);

		pipeline_create.get<vk::PipelineRenderingCreateInfo>()
			.setColorAttachmentFormats(device.surface_format.format)
			.setDepthAttachmentFormat(device.depth_format);

		default_pipeline = device->createGraphicsPipeline({}, pipeline_create.get()).value;

		device->destroy(vertex_shader);
		device->destroy(fragment_shader);
	}
}

Render::~Render() {
	device->waitIdle();

	device->destroy(default_pipeline);
	device->destroy(layout);
}

void Render::renderFrame(FrameInfo frame_info) {

	auto& cmd = render_cmd.get();

	auto image = swapchain.acquireImage(cmd.acquire_semaphore);

	if (storage.frame_extent != swapchain.get_extent()) [[unlikely]] {
		storage.resize_frame(swapchain.get_extent());
		aspect = static_cast<f32>(storage.frame_extent.width) / static_cast<f32>(storage.frame_extent.height);
	}

	{
		const Camera& camera = frame_info.camera;
		Matrix4 proj = Matrix4::perspective(camera.fov, aspect, camera.near_clip);
		Matrix4 view = Matrix4::lookAt(camera.eye, camera.target, camera.up);
		Uniform uniform{proj * view};
	}

	{
		std::vector<vk::ImageMemoryBarrier2> image_barrier(2);
		image_barrier[0]
			.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
			.setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
			.setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
			.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setImage(image)
			.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLevelCount(1)
			.setLayerCount(1);
		image_barrier[1]
			.setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests)
			.setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
			.setNewLayout(vk::ImageLayout::eDepthAttachmentOptimal)
			.setImage(storage.depth_buffer)
			.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth)
			.setLevelCount(1)
			.setLayerCount(1);

		cmd->pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
	}

	{
		vk::Viewport viewport(0, 0, storage.frame_extent.width, storage.frame_extent.height, 0, 1);
		cmd->setViewport(0, viewport);
		vk::Rect2D scissors({0, 0}, storage.frame_extent);
		cmd->setScissor(0, scissors);

		vk::RenderingAttachmentInfo colour_attachment(image, vk::ImageLayout::eColorAttachmentOptimal);
		colour_attachment.setLoadOp(vk::AttachmentLoadOp::eDontCare).setStoreOp(vk::AttachmentStoreOp::eStore);
#if 1
		vk::ClearValue clear_value(vk::ClearColorValue(std::array<float, 4>({{0.0, 0.3, 0.8, 1.0}})));
		colour_attachment.setLoadOp(vk::AttachmentLoadOp::eClear).setClearValue(clear_value);
#endif
		vk::RenderingAttachmentInfo depth_attachment(
			storage.depth_buffer.view, vk::ImageLayout::eDepthAttachmentOptimal);
		depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setClearValue(vk::ClearValue(vk::ClearDepthStencilValue(0)))
			.setStoreOp(vk::AttachmentStoreOp::eDontCare);
		vk::RenderingInfo rendering_info({}, scissors, 1, 0, colour_attachment, &depth_attachment);
		cmd->beginRendering(rendering_info);
	}

	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, default_pipeline);

	vk::DeviceSize offset = 0;
	cmd->bindVertexBuffers(0, storage.vertex_buffer.buffer, offset);

	cmd->draw(480, 1, 0, 0);

	cmd->endRendering();

	{
		vk::ImageMemoryBarrier2 image_barrier(
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, {}, vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR, device.graphics_queue.family, device.graphics_queue.family, image,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
		cmd->pipelineBarrier2(vk::DependencyInfo({}, {}, {}, image_barrier));
	}

	render_cmd.submit(cmd);

	swapchain.present(cmd.rendering_semaphore, image);
}

void Render::setModelCache(const ModelCache& mc) {
	storage.update_vertex_buffer(mc.vertices.data(), vectorSize(mc.vertices));
}

} // namespace Vulkan
