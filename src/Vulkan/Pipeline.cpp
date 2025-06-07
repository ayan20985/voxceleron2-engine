#include <array>
#include <chrono>
#include "../Oreginum//Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Pipeline.hpp"

Oreginum::Vulkan::Pipeline::Pipeline(std::shared_ptr<Device> device, const glm::uvec2& resolution,
	const Render_Pass& render_pass, const Shader& shader,
	std::vector<vk::DescriptorSetLayout> descriptor_set_layouts,
	uint8_t render_pass_number, const Pipeline& base)
	: device(device), descriptor_set_layouts(descriptor_set_layouts)
{
	auto start_time = std::chrono::high_resolution_clock::now();
	
	Logger::info("Creating Vulkan graphics pipeline for render pass " + std::to_string(render_pass_number) +
		", resolution " + std::to_string(resolution.x) + "x" + std::to_string(resolution.y) +
		", descriptor sets " + std::to_string(descriptor_set_layouts.size()), true);
	
	Logger::info("Shader stages: " + std::to_string(shader.get().size()));
	//Vertex input
	Logger::info("Configuring vertex input for render pass " + std::to_string(render_pass_number));
	std::vector<vk::VertexInputBindingDescription> binding_descriptions;
	std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;

	if(render_pass_number == 0)
	{
		Logger::info("G-Buffer pass: 4 vertex attributes (position, UVs, normals, material)");
		binding_descriptions.resize(1);
		binding_descriptions[0].setBinding(0);
		binding_descriptions[0].setStride(sizeof(float)*9);
		binding_descriptions[0].setInputRate(vk::VertexInputRate::eVertex);

		attribute_descriptions.resize(4);

		//Vertex
		attribute_descriptions[0].setBinding(0);
		attribute_descriptions[0].setLocation(0);
		attribute_descriptions[0].setFormat(vk::Format::eR32G32B32Sfloat);
		attribute_descriptions[0].setOffset(0);

		//UVs
		attribute_descriptions[1].setBinding(0);
		attribute_descriptions[1].setLocation(1);
		attribute_descriptions[1].setFormat(vk::Format::eR32G32Sfloat);
		attribute_descriptions[1].setOffset(sizeof(float)*3);

		//Normals
		attribute_descriptions[2].setBinding(0);
		attribute_descriptions[2].setLocation(2);
		attribute_descriptions[2].setFormat(vk::Format::eR32G32B32Sfloat);
		attribute_descriptions[2].setOffset(sizeof(float)*5);

		//Voxel material
		attribute_descriptions[3].setBinding(0);
		attribute_descriptions[3].setLocation(3);
		attribute_descriptions[3].setFormat(vk::Format::eR32Sfloat);
		attribute_descriptions[3].setOffset(sizeof(float)*8);
	}

	if(render_pass_number == 1 || render_pass_number == 2)
	{
		Logger::info("Shadow/Depth pass: 1 vertex attribute (position only)");
		binding_descriptions.resize(1);
		binding_descriptions[0].setBinding(0);
		binding_descriptions[0].setStride(sizeof(float)*9);
		binding_descriptions[0].setInputRate(vk::VertexInputRate::eVertex);

		attribute_descriptions.resize(1);

		//Vertex
		attribute_descriptions[0].setBinding(0);
		attribute_descriptions[0].setLocation(0);
		attribute_descriptions[0].setFormat(vk::Format::eR32G32B32Sfloat);
		attribute_descriptions[0].setOffset(0);
	}

	vk::PipelineVertexInputStateCreateInfo vertex_input_state_information;
	vertex_input_state_information.setVertexBindingDescriptionCount(
		static_cast<uint32_t>(binding_descriptions.size()));
	vertex_input_state_information.setPVertexBindingDescriptions(binding_descriptions.data());
	vertex_input_state_information.setVertexAttributeDescriptionCount(
		static_cast<uint32_t>(attribute_descriptions.size()));
	vertex_input_state_information.setPVertexAttributeDescriptions(
		attribute_descriptions.data());

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_information;
	input_assembly_state_information.setTopology(vk::PrimitiveTopology::eTriangleList);
	input_assembly_state_information.setPrimitiveRestartEnable(VK_FALSE);

	//Viewport
	Logger::info("Setting viewport: " + std::to_string(resolution.x) + "x" + std::to_string(resolution.y));
	vk::Viewport viewport;
	viewport.setX(0);
	viewport.setY(0);
	viewport.setWidth(static_cast<float>(resolution.x));
	viewport.setHeight(static_cast<float>(resolution.y));
	viewport.setMinDepth(0.f);
	viewport.setMaxDepth(1.f);

	vk::Rect2D scissor;
	scissor.setOffset({0, 0});
	scissor.setExtent(vk::Extent2D{resolution.x, resolution.y});

	vk::PipelineViewportStateCreateInfo viewport_state_information;
	viewport_state_information.setViewportCount(1);
	viewport_state_information.setPViewports(&viewport);
	viewport_state_information.setScissorCount(1);
	viewport_state_information.setPScissors(&scissor);

	//Rasterization
	bool clockwise_winding = render_pass_number >= 3;
	Logger::info("Rasterization: back face culling, " + std::string(clockwise_winding ? "clockwise" : "counter-clockwise") + " winding");
	vk::PipelineRasterizationStateCreateInfo rasterization_state_information;
	rasterization_state_information.setDepthClampEnable(VK_FALSE);
	rasterization_state_information.setRasterizerDiscardEnable(VK_FALSE);
	rasterization_state_information.setPolygonMode(vk::PolygonMode::eFill);
	rasterization_state_information.setCullMode(vk::CullModeFlagBits::eBack);
	rasterization_state_information.setFrontFace(clockwise_winding
		? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise);
	rasterization_state_information.setDepthBiasEnable(VK_FALSE);
	rasterization_state_information.setDepthBiasConstantFactor(0);
	rasterization_state_information.setDepthBiasClamp(0);
	rasterization_state_information.setDepthBiasSlopeFactor(0);
	rasterization_state_information.setLineWidth(1);

	//Multisampling
	vk::SampleCountFlagBits samples = (render_pass_number == 0 || render_pass_number == 2 ||
		render_pass_number == 5) ? Swapchain::SAMPLES : vk::SampleCountFlagBits::e1;
	Logger::info("Multisampling: " + std::to_string(static_cast<int>(samples)) + " samples");
	vk::PipelineMultisampleStateCreateInfo multisample_state_information;
	multisample_state_information.setRasterizationSamples(samples);
	multisample_state_information.setSampleShadingEnable(false);
	multisample_state_information.setMinSampleShading(0);
	multisample_state_information.setPSampleMask(nullptr);
	multisample_state_information.setAlphaToCoverageEnable(false);
	multisample_state_information.setAlphaToOneEnable(false);

	//Blending
	vk::PipelineColorBlendAttachmentState color_blend_attachment_state;
	color_blend_attachment_state.setBlendEnable(false);
	color_blend_attachment_state.setColorWriteMask(vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA);
	color_blend_attachment_state.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
	color_blend_attachment_state.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
	color_blend_attachment_state.setColorBlendOp(vk::BlendOp::eAdd);
	color_blend_attachment_state.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
	color_blend_attachment_state.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
	color_blend_attachment_state.setAlphaBlendOp(vk::BlendOp::eAdd);

	std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;
	if(render_pass_number == 0) {
		color_blend_attachments = {color_blend_attachment_state, color_blend_attachment_state,
			color_blend_attachment_state, color_blend_attachment_state};
		Logger::info("Color blending: 4 attachments (G-Buffer)");
	} else if(render_pass_number != 1) {
		color_blend_attachments = {color_blend_attachment_state};
		Logger::info("Color blending: 1 attachment");
	} else {
		Logger::info("Color blending: depth-only pass, no color attachments");
	}

	vk::PipelineColorBlendStateCreateInfo color_blend_state_information;
	color_blend_state_information.setLogicOpEnable(VK_FALSE);
	color_blend_state_information.setLogicOp(vk::LogicOp::eCopy);
	color_blend_state_information.setAttachmentCount(
		static_cast<uint32_t>(color_blend_attachments.size()));
	color_blend_state_information.setPAttachments(color_blend_attachments.data());
	color_blend_state_information.setBlendConstants({0, 0, 0, 0});

	//Depth stencil
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_information;
	if(render_pass_number < 3)
	{
		Logger::info("Depth testing: enabled, write enabled, less comparison");
		depth_stencil_information.setDepthTestEnable(VK_TRUE);
		depth_stencil_information.setDepthWriteEnable(VK_TRUE);
		depth_stencil_information.setDepthCompareOp(vk::CompareOp::eLess);
	} else {
		Logger::info("Depth testing: disabled for post-processing");
	}

	//Layout
	Logger::info("Creating pipeline layout with " + std::to_string(descriptor_set_layouts.size()) + " descriptor set layouts");
	vk::PipelineLayoutCreateInfo layout_information;
	layout_information.setSetLayoutCount(static_cast<uint32_t>(descriptor_set_layouts.size()));
	layout_information.setPSetLayouts(descriptor_set_layouts.data());
	layout_information.setPushConstantRangeCount(0);
	layout_information.setPPushConstantRanges(nullptr);

	vk::Result layout_result = device->get().createPipelineLayout(&layout_information, nullptr, &pipeline_layout);
	if(layout_result != vk::Result::eSuccess) {
		Logger::excep("Failed to create pipeline layout: VkResult " + std::to_string(static_cast<int>(layout_result)));
		Oreginum::Core::error("Could not create a Vulkan graphics pipeline layout.");
	}
	Logger::info("Pipeline layout created successfully");

	//Pipeline
	Logger::info("Creating graphics pipeline with render pass compatibility");
	if(base.get()) {
		Logger::info("Using base pipeline for optimization");
	}
	
	vk::GraphicsPipelineCreateInfo pipeline_information;
	pipeline_information.setStageCount(static_cast<uint32_t>(shader.get().size()));
	pipeline_information.setPStages(shader.get().data());
	pipeline_information.setPVertexInputState(&vertex_input_state_information);
	pipeline_information.setPInputAssemblyState(&input_assembly_state_information);
	pipeline_information.setPTessellationState(nullptr);
	pipeline_information.setPViewportState(&viewport_state_information);
	pipeline_information.setPRasterizationState(&rasterization_state_information);
	pipeline_information.setPMultisampleState(&multisample_state_information);
	pipeline_information.setPDepthStencilState(&depth_stencil_information);
	pipeline_information.setPColorBlendState(&color_blend_state_information);
	pipeline_information.setPDynamicState(nullptr);
	pipeline_information.setLayout(pipeline_layout);
	pipeline_information.setRenderPass(render_pass.get());
	pipeline_information.setSubpass(0);
	pipeline_information.setBasePipelineHandle(base.get());
	pipeline_information.setBasePipelineIndex(-1);

	auto pipeline_start = std::chrono::high_resolution_clock::now();
	vk::Result pipeline_result = device->get().createGraphicsPipelines(nullptr, 1,
		&pipeline_information, nullptr, pipeline.get());
	auto pipeline_end = std::chrono::high_resolution_clock::now();
	
	auto pipeline_duration = std::chrono::duration_cast<std::chrono::milliseconds>(pipeline_end - pipeline_start);
	
	if(pipeline_result != vk::Result::eSuccess) {
		Logger::excep("Failed to create graphics pipeline: VkResult " + std::to_string(static_cast<int>(pipeline_result)));
		Oreginum::Core::error("Could not create a Vulkan graphics pipeline.");
	}
	
	auto total_end = std::chrono::high_resolution_clock::now();
	auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - start_time);
	
	Logger::info("Graphics pipeline created successfully in " + std::to_string(pipeline_duration.count()) + "ms", true);
	Logger::info("Total pipeline creation time: " + std::to_string(total_duration.count()) + "ms");
}

Oreginum::Vulkan::Pipeline::~Pipeline()
{
	if(pipeline.use_count() != 1 || !device) {
		Logger::info("Pipeline destructor: shared pipeline or invalid device, skipping cleanup");
		return;
	}
	
	Logger::info("Destroying Vulkan graphics pipeline and layout", true);
	
	if(*pipeline) {
		Logger::info("Destroying graphics pipeline");
		device->get().destroyPipeline(*pipeline);
	}
	if(pipeline_layout) {
		Logger::info("Destroying pipeline layout");
		device->get().destroyPipelineLayout(pipeline_layout);
	}
	
	Logger::info("Pipeline cleanup completed");
}

void Oreginum::Vulkan::Pipeline::swap(Pipeline *other)
{
	std::swap(device, other->device);
	std::swap(pipeline_layout, other->pipeline_layout);
	std::swap(pipeline, other->pipeline);
	std::swap(descriptor_set_layouts, other->descriptor_set_layouts);
}