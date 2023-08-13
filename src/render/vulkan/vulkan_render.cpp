#include "vulkan_render.hpp"

#include "log.hpp"
#include "shaders.hpp"

namespace Vulkan {

template <class T> inline size_t vectorSize(std::vector<T> v) { return v.size() * sizeof(T); }

Render::Render(Context::Create c)
	: context(c), device(context), framebuffer(context.surface, device), storage(device), uniform_buffer(device),
	  cmd(device, device.graphics_queue) {

	{
		vk::PipelineLayoutCreateInfo layout_info({}, uniform_buffer.uniform_layout);
		pipeline_layout = device->createPipelineLayout(layout_info);
	}
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
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(ModelCache::Vertex, pos)),
			vk::VertexInputAttributeDescription(
				1, 0, vk::Format::eR32G32B32Sfloat, offsetof(ModelCache::Vertex, normal)),
			vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(ModelCache::Vertex, uv))};
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
			.setLayout(pipeline_layout);

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
	device->destroy(pipeline_layout);
}

void Render::renderFrame(FrameInfo frame_info) {

	cmd.begin();

	{
		const Camera& camera = frame_info.camera;
		mat4 proj = mat4::perspective(camera.fov, aspect, camera.near_clip);
		mat4 view = mat4::lookAt(camera.eye, camera.target, camera.up);
		Uniform uniform{proj * view};
		auto bind_target = uniform_buffer.update_uniform(uniform, cmd.get_index());
		cmd->bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, bind_target.first, bind_target.second);
	}

	framebuffer.start_rendering(cmd);

	storage.bind_buffers(cmd, cmd.get_index());

	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, default_pipeline);

	const auto& meshes = models.models.front().meshes;

	for (auto& m : meshes) {
		cmd->draw(m.num_vertices, 1, m.first_vertex, 0);
	}

	framebuffer.present(cmd);
}

void Render::setModelCache(const ModelCache& mc) {
	models = mc;
	storage.update_vertex_buffer(mc.vertices.data(), vectorSize(mc.vertices));
}

} // namespace Vulkan
