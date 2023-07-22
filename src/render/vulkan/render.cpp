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
		storage.update_uniform(uniform, render_cmd.get_index());
	}

	storage.start_render(cmd, image);

	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, default_pipeline);

	cmd->draw(480, 1, 0, 0);

	storage.end_render(cmd, image);

	render_cmd.submit(cmd);

	swapchain.present(cmd.rendering_semaphore, image);
}

void Render::setModelCache(const ModelCache& mc) {
	storage.update_vertex_buffer(mc.vertices.data(), vectorSize(mc.vertices));
}

} // namespace Vulkan
