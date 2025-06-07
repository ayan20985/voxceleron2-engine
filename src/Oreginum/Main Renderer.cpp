#include <iostream>
#include <random>
#include <GLM/gtc/matrix_transform.hpp>
#include "Renderer Core.hpp"
#include "Core.hpp"
#include "Logger.hpp"
#include "Renderable.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "../Tetra/Common.hpp"
#include "Main Renderer.hpp"

namespace
{
	Oreginum::Vulkan::Image depth_image, shadow_depth_image, position_image, normal_image,
		albedo_image, specular_image, translucent_image, ssao_noise_image, ssao_image, ssao_blur_image,
		lighting_multisample_image, lighting_image, bloom_blur_horizontal_image, bloom_blur_image;
	std::vector<Oreginum::Vulkan::Command_Buffer> command_buffers;
	Oreginum::Vulkan::Render_Pass g_buffer_render_pass, shadow_depth_render_pass,
		translucent_render_pass, ssao_render_pass, ssao_blur_render_pass,
		lighting_render_pass, bloom_blur_render_pass, composition_render_pass;
	std::vector<Oreginum::Vulkan::Framebuffer> g_buffer_framebuffers,
		shadow_depth_framebuffers, translucent_framebuffers, ssao_framebuffers,
		ssao_blur_framebuffers, lighting_framebuffers, bloom_blur_horizontal_framebuffers,
		bloom_blur_framebuffers, composition_framebuffers;
	Oreginum::Vulkan::Swapchain swapchain;
	Oreginum::Vulkan::Semaphore image_available, render_finished;
	Oreginum::Vulkan::Pipeline g_buffer_pipeline, shadow_depth_pipeline, translucent_pipeline,
		ssao_pipeline, ssao_blur_pipeline, lighting_pipeline, bloom_blur_pipeline, composition_pipeline;
	Oreginum::Vulkan::Descriptor_Set shadow_depth_descriptor_set,
		ssao_descriptor_set, ssao_blur_descriptor_set, lighting_descriptor_set,
		bloom_blur_initial_descriptor_set, bloom_blur_horizontal_descriptor_set,
		bloom_blur_descriptor_set, composition_descriptor_set;
	Oreginum::Vulkan::Sampler sampler, shadow_depth_sampler, ssao_noise_sampler;
	Oreginum::Vulkan::Buffer ssao_uniforms_buffer, lighting_uniforms_buffer, shadow_matrix_buffer,
		bloom_blur_initial_buffer, bloom_blur_horizontal_buffer, bloom_blur_buffer;
	std::vector<vk::SubpassDependency> render_pass_dependencies;
	glm::fmat4 shadow_matrix;

	struct Lighting_Uniforms
	{
		glm::fvec4 camera_position;
		glm::fmat4 inverse_view;
		glm::fmat4 transposed_view;
		glm::fmat4 shadow_matrix;
	};
	
	constexpr uint8_t SSAO_KERNEL_SIZE{8};
	glm::vec4 ssao_kernel[SSAO_KERNEL_SIZE];
	struct SSAO_Uniforms
	{
		glm::vec4 kernel[SSAO_KERNEL_SIZE];
		glm::fmat4 projection_matrix;
	};
	const glm::u16vec2 SHADOW_DEPTH_BUFFER_RESOLUTION{8192};
	constexpr uint8_t BLOOM_DERESOLUTION{10};
	constexpr uint8_t BLOOM_ITERATIONS{3};
	glm::u16vec2 bloom_resolution;

	float lerp(float a, float b, float f){ return a+f*(b-a); }

	//Helper functions
	void create_buffer(Oreginum::Vulkan::Buffer *buffer, uint32_t size)
	{
		uint32_t padded_uniform_size{Oreginum::Renderer_Core::get_padded_uniform_size(size)};
		*buffer = {Oreginum::Renderer_Core::get_device(),
			Oreginum::Renderer_Core::get_temporary_command_buffer(),
			vk::BufferUsageFlagBits::eUniformBuffer, padded_uniform_size};
	}

	void write_buffer(Oreginum::Vulkan::Buffer *buffer, void *data, uint32_t size)
	{
		uint32_t padded_uniform_size{Oreginum::Renderer_Core::get_padded_uniform_size(size)};
		void *temporary_buffer{std::malloc(padded_uniform_size)};
		std::memcpy(static_cast<uint8_t *>(temporary_buffer), data, size);
		buffer->write(temporary_buffer, padded_uniform_size);
		std::free(temporary_buffer);
	}

	void create_and_write_buffer(Oreginum::Vulkan::Buffer *buffer, void *data, uint32_t size)
	{
		create_buffer(buffer, size);
		write_buffer(buffer, data, size);
	}

	enum Attachment_Type{DEPTH, POSITION, RGB, SPECULAR, SSAO, SHADOW_DEPTH,
		DEPTH_TRANSLUCENT, HDR, HDR_RESOLVE, HDR_MULTISAMPLE, BLOOM, SWAPCHAIN};

	void create_render_pass(Oreginum::Vulkan::Render_Pass *render_pass,
		const std::vector<Attachment_Type>& attachment_types)
	{
		//Attachments
		std::vector<vk::AttachmentDescription> attachments(attachment_types.size());
		std::vector<vk::AttachmentReference> color_attachment_references;
		vk::AttachmentReference resolve_attachment_reference;
		vk::AttachmentReference depth_attachment_reference;
		bool has_depth{}, has_resolve{};

		for(uint8_t i{}; i < attachment_types.size(); ++i)
		{
			vk::Format format;
			vk::SampleCountFlagBits samples{Oreginum::Vulkan::Swapchain::SAMPLES};
			vk::AttachmentLoadOp load_op{vk::AttachmentLoadOp::eClear};
			vk::AttachmentStoreOp store_op{vk::AttachmentStoreOp::eStore};
			vk::ImageLayout layout{vk::ImageLayout::eUndefined};
			vk::ImageLayout final_layout{vk::ImageLayout::eShaderReadOnlyOptimal};

			switch(attachment_types[i])
			{
			case DEPTH_TRANSLUCENT:
				load_op = vk::AttachmentLoadOp::eLoad;
				store_op = vk::AttachmentStoreOp::eDontCare;
			case DEPTH:
				format = Oreginum::Vulkan::Image::DEPTH_FORMAT;
				layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				final_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			break;
			case POSITION: format = Oreginum::Vulkan::Image::POSITION_FORMAT; break;
			case RGB: format = Oreginum::Vulkan::Image::LINEAR_FORMAT; break;
			case SSAO: samples = vk::SampleCountFlagBits::e1;
			case SPECULAR: format = Oreginum::Vulkan::Image::MONOCHROME_FORMAT; break;
			case SHADOW_DEPTH:
				format = Oreginum::Vulkan::Image::DEPTH_FORMAT;
				samples = vk::SampleCountFlagBits::e1;
			break;
			case HDR: case HDR_RESOLVE: samples = vk::SampleCountFlagBits::e1;
			case HDR_MULTISAMPLE: format = Oreginum::Vulkan::Image::HDR_FORMAT; break;
			case BLOOM:
				samples = vk::SampleCountFlagBits::e1;
				format = Oreginum::Vulkan::Image::HDR_FORMAT;
			break;
			case SWAPCHAIN:
				samples = vk::SampleCountFlagBits::e1;
				format = Oreginum::Vulkan::Image::SWAPCHAIN_FORMAT;
				layout = vk::ImageLayout::ePresentSrcKHR;
				final_layout = vk::ImageLayout::ePresentSrcKHR;
			break;
			}

			attachments[i] = {vk::AttachmentDescriptionFlags{}, format, samples, load_op,
				store_op, load_op, store_op, layout, final_layout};

			if(attachment_types[i] == DEPTH || attachment_types[i] == SHADOW_DEPTH ||
				attachment_types[i] == DEPTH_TRANSLUCENT)
			{
				has_depth = true;
				depth_attachment_reference = {i, vk::ImageLayout::eDepthStencilAttachmentOptimal};
			}
			else if(attachment_types[i] == HDR_RESOLVE)
			{
				has_resolve = true;
				resolve_attachment_reference = {i, vk::ImageLayout::eColorAttachmentOptimal};
			}
			else color_attachment_references.emplace_back(
				i, vk::ImageLayout::eColorAttachmentOptimal);
		}

		//Subpass
		std::vector<vk::SubpassDescription> subpasses(1);
		subpasses[0] = {vk::SubpassDescriptionFlags{}, vk::PipelineBindPoint::eGraphics, 0,
			nullptr, static_cast<uint32_t>(color_attachment_references.size()),
			color_attachment_references.data(),
			has_resolve ? &resolve_attachment_reference : nullptr,
			has_depth ? &depth_attachment_reference : nullptr, 0, nullptr};

		//Create
		*render_pass = {Oreginum::Renderer_Core::get_device(), attachments,
			subpasses, render_pass_dependencies};
	}

	Oreginum::Vulkan::Descriptor_Set create_descriptor_set(uint8_t images, uint8_t uniforms = 0)
	{
		std::vector<std::pair<vk::DescriptorType, vk::ShaderStageFlags>> bindings;
		for(uint8_t i{}; i < images; ++i) bindings.emplace_back(
			vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
		for(uint8_t i{}; i < uniforms; ++i) bindings.emplace_back(
			vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);
		return {Oreginum::Renderer_Core::get_device(),
			Oreginum::Renderer_Core::get_static_descriptor_pool(), bindings};
	}

	void transition_to_depth(Oreginum::Vulkan::Image *image)
	{
		image->transition(Oreginum::Renderer_Core::get_temporary_command_buffer(),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal,
		vk::AccessFlags{}, vk::AccessFlagBits::eDepthStencilAttachmentRead |
		vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eEarlyFragmentTests);
	}

	void create_image(Oreginum::Vulkan::Image *image, vk::Format format,
		bool multisampled = true, const glm::uvec2& resolution = Oreginum::Window::get_resolution())
	{
		*image = Oreginum::Vulkan::Image{Oreginum::Renderer_Core::get_device(), sampler,
			resolution, vk::ImageUsageFlagBits::eColorAttachment |
			vk::ImageUsageFlagBits::eSampled, format, vk::ImageAspectFlagBits::eColor,
			multisampled ? Oreginum::Vulkan::Swapchain::SAMPLES : vk::SampleCountFlagBits::e1};

		image->transition(Oreginum::Renderer_Core::get_temporary_command_buffer(),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
		vk::AccessFlags{}, vk::AccessFlagBits::eColorAttachmentWrite,
		vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	}

	void begin_render_pass(const Oreginum::Vulkan::Render_Pass& render_pass,
		const std::vector<vk::ClearValue>& clear_values, uint8_t framebuffer,
		const std::vector<Oreginum::Vulkan::Framebuffer> framebuffers,
		const Oreginum::Vulkan::Pipeline& pipeline)
	{
		vk::RenderPassBeginInfo render_pass_begin_information{
			render_pass.get(), framebuffers[framebuffer].get(), vk::Rect2D{{0, 0},
			vk::Extent2D{framebuffers[framebuffer].get_resolution().x,
			framebuffers[framebuffer].get_resolution().y}},
			static_cast<uint32_t>(clear_values.size()), clear_values.data()};
		command_buffers.back().get().beginRenderPass(
			render_pass_begin_information, vk::SubpassContents::eInline);
		command_buffers.back().get().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
	}

	void geometry_render(uint8_t index, bool depth_buffer, uint8_t color_attachment_count,
		const Oreginum::Vulkan::Render_Pass& render_pass,
		const std::vector<Oreginum::Vulkan::Framebuffer> framebuffers,
		const Oreginum::Vulkan::Pipeline& pipeline, bool translucent,
		std::vector<vk::DescriptorSet> descriptor_sets,
		bool use_renderable_descriptor = false)
	{
		//Clear values
			std::vector<vk::ClearValue> clear_values;
		if(depth_buffer){ clear_values.emplace_back();
			clear_values.back().setDepthStencil({1, 0}); }
		for(uint8_t i{}; i < color_attachment_count; ++i) clear_values.emplace_back();

		//Begin
		begin_render_pass(render_pass, clear_values, index, framebuffers, pipeline);

		//Render
		uint32_t count{};
		auto iterator{Oreginum::Renderer_Core::get_renderables().begin()};
		while(iterator != Oreginum::Renderer_Core::get_renderables().end())
		{
			if(iterator->second->get_type() == (translucent ?
				Oreginum::Renderable::Type::VOXEL_TRANSLUCENT : Oreginum::Renderable::Type::VOXEL))
			{
				if(use_renderable_descriptor)
					descriptor_sets.emplace_back(iterator->second->get_descriptor_set().get());

				command_buffers.back().get().bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
					pipeline.get_layout(), 0, descriptor_sets,
					{count*Oreginum::Renderer_Core::get_padded_uniform_size()});

				iterator->second->draw(command_buffers.back());

				if(use_renderable_descriptor) descriptor_sets.pop_back();
			}
			++iterator, ++count;
		}
		command_buffers.back().get().endRenderPass();
	}

	void deferred_render(uint8_t index, uint8_t color_attachment_count,
		const Oreginum::Vulkan::Render_Pass& render_pass,
		const std::vector<Oreginum::Vulkan::Framebuffer> framebuffers,
		const Oreginum::Vulkan::Pipeline& pipeline,
		const vk::DescriptorSet& descriptor_set)
	{
		//Clear values
		std::vector<vk::ClearValue> clear_values;
		for(uint8_t i{}; i < color_attachment_count; ++i) clear_values.emplace_back();

		//Begin
		begin_render_pass(render_pass, clear_values, index, framebuffers, pipeline);

		//Render
		if(Oreginum::Renderer_Core::get_renderables().size())
		{
			command_buffers.back().get().bindPipeline(
				vk::PipelineBindPoint::eGraphics, pipeline.get());
			command_buffers.back().get().bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				pipeline.get_layout(), 0, descriptor_set, {});
			command_buffers.back().get().draw(3, 1, 0, 0);
		}
		command_buffers.back().get().endRenderPass();
	}
}

void Oreginum::Main_Renderer::initialize()
{
	Logger::info("=== Initializing Main Renderer ===");
	auto start_time = std::chrono::high_resolution_clock::now();
	
	auto window_res = Window::get_resolution();
	bloom_resolution = glm::fvec2(window_res)/static_cast<float>(BLOOM_DERESOLUTION);
	Logger::info("Bloom resolution calculated: " + std::to_string(bloom_resolution.x) + "x" +
		std::to_string(bloom_resolution.y) + " (1/" + std::to_string(BLOOM_DERESOLUTION) + " of " +
		std::to_string(window_res.x) + "x" + std::to_string(window_res.y) + ")");

	Logger::info("Creating swapchain");
	swapchain = {*Renderer_Core::get_instance(), Renderer_Core::get_surface(),
		Renderer_Core::get_device(), Renderer_Core::get_temporary_command_buffer()};

	Logger::info("Creating synchronization semaphores");
	image_available = {Renderer_Core::get_device()};
	render_finished = {Renderer_Core::get_device()};

	//Samplers
	Logger::info("Creating samplers for texture filtering");
	sampler = {Renderer_Core::get_device(), 0, false, vk::SamplerAddressMode::eClampToEdge};
	ssao_noise_sampler = {Renderer_Core::get_device()};
	shadow_depth_sampler = {Renderer_Core::get_device(), 0, false,
		vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear, vk::Filter::eLinear};

	//Initialize descriptor sets
	Logger::info("Creating descriptor sets for rendering passes");
	shadow_depth_descriptor_set = {Renderer_Core::get_device(),
		Renderer_Core::get_static_descriptor_pool(),
		{{vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex}}};
	ssao_descriptor_set = {create_descriptor_set(3, 1)};
	ssao_blur_descriptor_set = {create_descriptor_set(1)};
	lighting_descriptor_set = {create_descriptor_set(7, 2)};
	bloom_blur_initial_descriptor_set = {create_descriptor_set(1, 1)};
	bloom_blur_horizontal_descriptor_set = {create_descriptor_set(1, 1)};
	bloom_blur_descriptor_set = {create_descriptor_set(1, 1)};
	composition_descriptor_set = {create_descriptor_set(2)};

	Logger::info("Creating render passes and graphics pipelines");
	create_render_passes_and_pipelines();
	
	Logger::info("Creating images and framebuffers");
	create_images_and_framebuffers();

	//Shadow matrix buffer
	float half_world_width{Tetra::WORLD_SIZE.x*Tetra::CHUNK_SIZE/2.f};
	glm::fmat4 shadow_projection_matrix{
		glm::ortho(half_world_width, -half_world_width,
		-half_world_width, half_world_width, 0.f, 4096.f)};

	glm::fvec3 position{0.f, -1.f, -1.f};
	position *= 2048;
	glm::fvec3 world_up{0, 0, 1};
	glm::fmat4 shadow_view_matrix{glm::lookAt(position, {0, 0, 0}, world_up)};

	shadow_matrix = shadow_projection_matrix*shadow_view_matrix;
	uint32_t shadow_matrix_size{sizeof(glm::fmat4)};
	create_and_write_buffer(&shadow_matrix_buffer, &shadow_matrix, shadow_matrix_size);

	//Shadow descriptor set
	shadow_depth_descriptor_set.write({&shadow_matrix_buffer});

	//SSAO kernel buffer
	std::uniform_real_distribution<float> distribution(0.f, 1.f);
	std::random_device device;
	std::default_random_engine generator;

	for(uint8_t i{}; i < SSAO_KERNEL_SIZE; ++i)
	{
		glm::vec3 sample{distribution(generator)*2.f-1.f,
			distribution(generator)*2.f-1.f, distribution(generator)};
		sample = glm::normalize(sample);
		sample *= distribution(generator);

		float scale{static_cast<float>(i)/SSAO_KERNEL_SIZE};
		scale = lerp(.1f, 1.f, scale*scale);
		ssao_kernel[i] = glm::fvec4{sample*scale, 0.f};
	}
	create_buffer(&ssao_uniforms_buffer, sizeof(SSAO_Uniforms));

	//SSAO noise images
	std::vector<glm::fvec4> ssao_noise(16);
	for(uint8_t i{}; i < 16; ++i) ssao_noise[i] = glm::fvec4(distribution(generator)*
		2.f-1.f, distribution(generator)*2.f-1.f, 0.f, 0.f);
	ssao_noise_image = {Renderer_Core::get_device(), ssao_noise_sampler,
		Renderer_Core::get_temporary_command_buffer(), glm::uvec2{4, 4},
		{ssao_noise.data()}, vk::Format::eR32G32B32A32Sfloat, false};

	//Lighting buffer
	create_buffer(&lighting_uniforms_buffer, sizeof(Lighting_Uniforms));

	//Bloom buffers
	uint32_t bloom_blur_mode{0};
	create_and_write_buffer(&bloom_blur_initial_buffer, &bloom_blur_mode, sizeof(uint32_t));
	bloom_blur_mode = 1;
	create_and_write_buffer(&bloom_blur_horizontal_buffer, &bloom_blur_mode, sizeof(uint32_t));
	bloom_blur_mode = 2;
	create_and_write_buffer(&bloom_blur_buffer, &bloom_blur_mode, sizeof(uint32_t));

	write_descriptor_sets();
	
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
	Logger::info("=== Main Renderer initialization completed in " + std::to_string(duration.count()) + "ms ===");
}

void Oreginum::Main_Renderer::write_descriptor_sets()
{
	ssao_descriptor_set.write({&position_image, &normal_image, &ssao_noise_image, &ssao_uniforms_buffer});
	ssao_blur_descriptor_set.write({&ssao_image});
	lighting_descriptor_set.write({&position_image, &normal_image, &albedo_image, &specular_image,
		&translucent_image, &ssao_blur_image, &shadow_depth_image, &lighting_uniforms_buffer});
	bloom_blur_initial_descriptor_set.write({&lighting_image, &bloom_blur_initial_buffer});
	bloom_blur_horizontal_descriptor_set.write({&bloom_blur_image, &bloom_blur_horizontal_buffer});
	bloom_blur_descriptor_set.write({&bloom_blur_horizontal_image, &bloom_blur_buffer});
	composition_descriptor_set.write({&lighting_image, &bloom_blur_image});
}

void Oreginum::Main_Renderer::create_render_passes_and_pipelines()
{
	//Dependencies
	render_pass_dependencies.clear();
	render_pass_dependencies.resize(2);
	render_pass_dependencies[0] = {VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eMemoryRead,
		vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
		vk::DependencyFlagBits::eByRegion};
	render_pass_dependencies[1] = {0, VK_SUBPASS_EXTERNAL,
		vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eMemoryRead, vk::DependencyFlagBits::eByRegion};

	//Render passes
	create_render_pass(&g_buffer_render_pass, {DEPTH, POSITION, RGB, RGB, SPECULAR});
	create_render_pass(&shadow_depth_render_pass, {SHADOW_DEPTH});
	create_render_pass(&translucent_render_pass, {DEPTH_TRANSLUCENT, RGB});
	create_render_pass(&ssao_render_pass, {SSAO});
	create_render_pass(&ssao_blur_render_pass, {SSAO});
	if(Vulkan::Swapchain::MULTISAMPLE) create_render_pass(
		&lighting_render_pass, {HDR_RESOLVE, HDR_MULTISAMPLE});
	else create_render_pass(&lighting_render_pass, {HDR});
	create_render_pass(&bloom_blur_render_pass, {BLOOM});
	create_render_pass(&composition_render_pass, {SWAPCHAIN});

	//Pipelines
	std::vector<vk::DescriptorSetLayout> g_buffer_descriptor_set_layout{
		Renderer_Core::get_uniform_descriptor_set().get_layout(),
		Renderer_Core::get_texture_descriptor_set().get_layout()};
	std::vector<vk::DescriptorSetLayout> shadow_depth_descriptor_set_layout{
		Renderer_Core::get_uniform_descriptor_set().get_layout(),
		shadow_depth_descriptor_set.get_layout()};
	std::vector<vk::DescriptorSetLayout> translucent_descriptor_set_layout{
		Renderer_Core::get_uniform_descriptor_set().get_layout()};

	g_buffer_pipeline = Renderer_Core::create_pipeline(Window::get_resolution(),
		g_buffer_render_pass, "G-Buffer Vertex", "G-Buffer Fragment", 0,
		g_buffer_descriptor_set_layout);
	shadow_depth_pipeline = Renderer_Core::create_pipeline(SHADOW_DEPTH_BUFFER_RESOLUTION,
		shadow_depth_render_pass, "Shadow Depth Vertex", "Shadow Depth Fragment", 1,
		shadow_depth_descriptor_set_layout);
	translucent_pipeline = Renderer_Core::create_pipeline(Window::get_resolution(),
		translucent_render_pass, "Translucent Vertex", "Translucent Fragment", 2,
		translucent_descriptor_set_layout);
	ssao_pipeline = Renderer_Core::create_pipeline(Window::get_resolution(),
		ssao_render_pass, "SSAO Vertex", Vulkan::Swapchain::MULTISAMPLE ?
		"SSAO Fragment Multisampled" : "SSAO Fragment", 3, {ssao_descriptor_set.get_layout()});
	ssao_blur_pipeline = Renderer_Core::create_pipeline(Window::get_resolution(),
		ssao_blur_render_pass, "SSAO Blur Vertex", "SSAO Blur Fragment", 4,
		{ssao_blur_descriptor_set.get_layout()});
	lighting_pipeline = Renderer_Core::create_pipeline(Window::get_resolution(),
		lighting_render_pass, "Lighting Vertex", Vulkan::Swapchain::MULTISAMPLE ?
			"Lighting Fragment Multisampled" : "Lighting Fragment", 5,
		{lighting_descriptor_set.get_layout()});
	bloom_blur_pipeline = Renderer_Core::create_pipeline(bloom_resolution,
		bloom_blur_render_pass, "Bloom Blur Vertex", "Bloom Blur Fragment", 6,
		{bloom_blur_descriptor_set.get_layout()});
	composition_pipeline = Renderer_Core::create_pipeline(Window::get_resolution(),
		composition_render_pass, "Composition Vertex", "Composition Fragment", 7,
		{composition_descriptor_set.get_layout()});
}

void Oreginum::Main_Renderer::create_images_and_framebuffers()
{
	//Images
	depth_image = Vulkan::Image{Renderer_Core::get_device(), sampler, Window::get_resolution(),
		vk::ImageUsageFlagBits::eDepthStencilAttachment, Vulkan::Image::DEPTH_FORMAT,
		vk::ImageAspectFlagBits::eDepth, Vulkan::Swapchain::SAMPLES};
	transition_to_depth(&depth_image);
	shadow_depth_image = Vulkan::Image{Renderer_Core::get_device(), shadow_depth_sampler,
		SHADOW_DEPTH_BUFFER_RESOLUTION, vk::ImageUsageFlagBits::eDepthStencilAttachment |
		vk::ImageUsageFlagBits::eSampled, Vulkan::Image::DEPTH_FORMAT,
		vk::ImageAspectFlagBits::eDepth, vk::SampleCountFlagBits::e1};
	transition_to_depth(&shadow_depth_image);
	create_image(&position_image, Vulkan::Image::POSITION_FORMAT);
	create_image(&normal_image, Vulkan::Image::LINEAR_FORMAT);
	create_image(&albedo_image, Vulkan::Image::LINEAR_FORMAT);
	create_image(&specular_image, Vulkan::Image::MONOCHROME_FORMAT);
	create_image(&translucent_image, Vulkan::Image::LINEAR_FORMAT);
	create_image(&ssao_image, Vulkan::Image::MONOCHROME_FORMAT, false);
	create_image(&ssao_blur_image, Vulkan::Image::MONOCHROME_FORMAT, false);
	create_image(&lighting_image, Vulkan::Image::HDR_FORMAT, false);
	create_image(&lighting_multisample_image, Vulkan::Image::HDR_FORMAT);
	create_image(&bloom_blur_horizontal_image, Vulkan::Image::HDR_FORMAT, false, bloom_resolution);
	create_image(&bloom_blur_image, Vulkan::Image::HDR_FORMAT, false, bloom_resolution);

	//Framebuffers
	g_buffer_framebuffers.clear();
	shadow_depth_framebuffers.clear();
	translucent_framebuffers.clear();
	ssao_framebuffers.clear();
	ssao_blur_framebuffers.clear();
	lighting_framebuffers.clear();
	bloom_blur_horizontal_framebuffers.clear();
	bloom_blur_framebuffers.clear();
	composition_framebuffers.clear();
	for(const auto& i : swapchain.get_images())
	{
		std::vector<const Vulkan::Image *> attachments;
		attachments = {&depth_image, &position_image,
			&normal_image, &albedo_image, &specular_image};
		g_buffer_framebuffers.push_back({Renderer_Core::get_device(),
			Window::get_resolution(), g_buffer_render_pass, attachments});
		attachments = {&shadow_depth_image};
		shadow_depth_framebuffers.push_back({Renderer_Core::get_device(),
			SHADOW_DEPTH_BUFFER_RESOLUTION, shadow_depth_render_pass, attachments});
		attachments = {&depth_image, &translucent_image};
		translucent_framebuffers.push_back({Renderer_Core::get_device(),
			Window::get_resolution(), translucent_render_pass, attachments});
		attachments = {&ssao_image};
		ssao_framebuffers.push_back({Renderer_Core::get_device(),
			Window::get_resolution(), ssao_render_pass, attachments});
		attachments = {&ssao_blur_image};
		ssao_blur_framebuffers.push_back({Renderer_Core::get_device(),
			Window::get_resolution(), ssao_blur_render_pass, attachments});
		if(Vulkan::Swapchain::MULTISAMPLE)
			attachments = {&lighting_image, &lighting_multisample_image};
		else attachments = {&lighting_image};
		lighting_framebuffers.push_back({Renderer_Core::get_device(),
			Window::get_resolution(), lighting_render_pass, attachments});
		attachments = {&bloom_blur_horizontal_image};
		bloom_blur_horizontal_framebuffers.push_back({Renderer_Core::get_device(),
			bloom_resolution, bloom_blur_render_pass, attachments});
		attachments = {&bloom_blur_image};
		bloom_blur_framebuffers.push_back({Renderer_Core::get_device(),
			bloom_resolution, bloom_blur_render_pass, attachments});
		attachments = {&i};
		composition_framebuffers.push_back({Renderer_Core::get_device(),
			Window::get_resolution(), composition_render_pass, attachments});
	}
}

void Oreginum::Main_Renderer::update_uniforms()
{
	Logger::info("Updating Main Renderer uniform buffers");
	
	//SSAO
	Logger::info("Updating SSAO uniforms with " + std::to_string(SSAO_KERNEL_SIZE) + " kernel samples");
	SSAO_Uniforms ssao_uniforms
	{{}, Oreginum::Camera::get_projection()};
	for(uint8_t i{}; i < SSAO_KERNEL_SIZE; ++i) ssao_uniforms.kernel[i] = ssao_kernel[i];
	write_buffer(&ssao_uniforms_buffer, &ssao_uniforms, sizeof(SSAO_Uniforms));
	Logger::info("SSAO uniforms buffer updated (" + std::to_string(sizeof(SSAO_Uniforms)) + " bytes)");

	//Lighting
	auto camera_pos = Oreginum::Camera::get_position();
	Logger::info("Updating lighting uniforms for camera at (" +
		std::to_string(camera_pos.x) + ", " + std::to_string(camera_pos.y) + ", " + std::to_string(camera_pos.z) + ")");
	Lighting_Uniforms lighting_uniforms
	{
		glm::fvec4{Oreginum::Camera::get_position(), 0.f},
		glm::inverse(Oreginum::Camera::get_view()),
		glm::transpose(Oreginum::Camera::get_view()),
		shadow_matrix
	};
	write_buffer(&lighting_uniforms_buffer, &lighting_uniforms, sizeof(Lighting_Uniforms));
	Logger::info("Lighting uniforms buffer updated (" + std::to_string(sizeof(Lighting_Uniforms)) + " bytes)");
}

void Oreginum::Main_Renderer::record()
{
	Logger::info("Recording Main Renderer command buffers");
	auto start_time = std::chrono::high_resolution_clock::now();
	
	command_buffers.clear();
	auto swapchain_images_count = swapchain.get_images().size();
	Logger::info("Recording commands for " + std::to_string(swapchain_images_count) + " swapchain images");
	
	for(uint8_t i{}; i < swapchain_images_count; ++i)
	{
		//Start
		Logger::info("Recording command buffer " + std::to_string(i + 1) + "/" + std::to_string(swapchain_images_count));
		command_buffers.push_back({Renderer_Core::get_device(),
			Renderer_Core::get_command_pool()});
		command_buffers.back().begin();

		//Normal render
		if(!Oreginum::Window::is_resizing())
		{
			Logger::info("Recording full deferred rendering pipeline for frame " + std::to_string(i));
			
			//Render passes
			Logger::info("Recording G-Buffer pass (geometry render with 4 color attachments)");
			geometry_render(i, true, 4, g_buffer_render_pass, g_buffer_framebuffers, g_buffer_pipeline,
				false, {Oreginum::Renderer_Core::get_uniform_descriptor_set().get()}, true);
				
			Logger::info("Recording shadow depth pass");
			geometry_render(i, true, 0, shadow_depth_render_pass, shadow_depth_framebuffers,
				shadow_depth_pipeline, false,
				{Oreginum::Renderer_Core::get_uniform_descriptor_set().get(),
				shadow_depth_descriptor_set.get()});
				
			Logger::info("Recording translucent geometry pass");
			geometry_render(i, false, 2, translucent_render_pass, translucent_framebuffers,
				translucent_pipeline, true, {Oreginum::Renderer_Core::get_uniform_descriptor_set().get()});
		
			Logger::info("Recording SSAO pass (screen-space ambient occlusion)");
			deferred_render(i, 1, ssao_render_pass, ssao_framebuffers,
				ssao_pipeline, ssao_descriptor_set.get());
				
			Logger::info("Recording SSAO blur pass");
			deferred_render(i, 1, ssao_blur_render_pass, ssao_blur_framebuffers,
				ssao_blur_pipeline, ssao_blur_descriptor_set.get());
				
			Logger::info("Recording lighting pass (deferred shading)");
			deferred_render(i, 2, lighting_render_pass, lighting_framebuffers,
				lighting_pipeline, lighting_descriptor_set.get());

			Logger::info("Recording bloom blur passes (" + std::to_string(BLOOM_ITERATIONS) + " iterations)");
			uint8_t mode{};
			for(uint8_t j{}; j < BLOOM_ITERATIONS*2+1; ++j)
			{
				std::string mode_name = (mode == 0) ? "initial" : (mode == 1) ? "horizontal" : "vertical";
				Logger::info("Recording bloom blur pass " + std::to_string(j + 1) + "/" +
					std::to_string(BLOOM_ITERATIONS*2+1) + " (" + mode_name + ")");
					
				deferred_render(i, 1, bloom_blur_render_pass, mode == 0 || mode == 2 ?
					bloom_blur_framebuffers : bloom_blur_horizontal_framebuffers, bloom_blur_pipeline,
					mode == 0 ? bloom_blur_initial_descriptor_set.get() :
					mode == 1 ? bloom_blur_horizontal_descriptor_set.get() :
					bloom_blur_descriptor_set.get());

				if(mode == 0 || mode == 2) mode = 1;
				else if(mode == 1) mode = 2;

				//Wait for the pass to finish before using the image in the next pass
				vk::MemoryBarrier barrier{vk::AccessFlagBits::eShaderWrite,
					vk::AccessFlagBits::eShaderRead};
				command_buffers.back().get().pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader,
					vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags{}, barrier,
					nullptr, nullptr);
			}

			Logger::info("Recording composition pass (final image composition)");
			deferred_render(i, 1, composition_render_pass, composition_framebuffers,
				composition_pipeline, composition_descriptor_set.get());
		}

		//Moving or resizing render
		else
		{
			Logger::info("Recording resize/move clear pass for frame " + std::to_string(i));
			swapchain.get_images()[i].transition(command_buffers.back(), vk::ImageLayout::ePresentSrcKHR,
				vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eMemoryRead,
				vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer, false);

			vk::ImageSubresourceRange subresource_range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
			command_buffers.back().get().clearColorImage(swapchain.get_images()[i].get(),
				vk::ImageLayout::eTransferDstOptimal, {}, {subresource_range});

			swapchain.get_images()[i].transition(command_buffers.back(),
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
				vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, false);
		}

		command_buffers.back().end();
	}
	
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
	Logger::info("Command buffer recording completed in " + std::to_string(duration.count()) + "μs");
}

void Oreginum::Main_Renderer::reinitialize_swapchain()
{
	Logger::info("Reinitializing Main Renderer swapchain");
	auto start_time = std::chrono::high_resolution_clock::now();
	
	swapchain.reinitialize(Renderer_Core::get_device(), Renderer_Core::get_temporary_command_buffer());
	
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
	Logger::info("Swapchain reinitialized in " + std::to_string(duration.count()) + "ms");
}

void Oreginum::Main_Renderer::render()
{
	auto start_time = std::chrono::high_resolution_clock::now();
	
	//Get swapchain image
	uint32_t image_index;
	vk::Result result{Renderer_Core::get_device()->get().acquireNextImageKHR(swapchain.get(),
		std::numeric_limits<uint64_t>::max(), image_available.get(), nullptr, &image_index)};
	if(result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
	{
		Logger::excep("Failed to acquire Vulkan swapchain image: " +
			std::string(result == vk::Result::eSuboptimalKHR ? "suboptimal" : "out of date"));
		Core::error("Could not aquire a Vulkan swapchain image.");
	}
	Logger::info("Acquired swapchain image " + std::to_string(image_index) + " for rendering");

	//Submit render commands
	auto submit_start = std::chrono::high_resolution_clock::now();
	Renderer_Core::submit_command_buffers({command_buffers[image_index].get()},
		{image_available.get()}, {vk::PipelineStageFlagBits::eColorAttachmentOutput},
		{render_finished.get()});
	auto submit_end = std::chrono::high_resolution_clock::now();
	auto submit_duration = std::chrono::duration_cast<std::chrono::microseconds>(submit_end - submit_start);
	Logger::info("Command buffer submitted to GPU in " + std::to_string(submit_duration.count()) + "μs");

	//Present swapchain image
	std::array<vk::Semaphore, 1> present_wait_semaphores{render_finished.get()};
	std::array<vk::SwapchainKHR, 1> swapchains{swapchain.get()};
	vk::PresentInfoKHR present_information{static_cast<uint32_t>(present_wait_semaphores.size()),
		present_wait_semaphores.data(), static_cast<uint32_t>(swapchains.size()), swapchains.data(),
		&image_index};

	auto present_start = std::chrono::high_resolution_clock::now();
	result = Renderer_Core::get_device()->get_present_queue().presentKHR(present_information);
	if(result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
	{
		Logger::excep("Failed to present to Vulkan queue: " +
			std::string(result == vk::Result::eSuboptimalKHR ? "suboptimal" : "out of date"));
		Core::error("Could not submit Vulkan presentation queue.");
	}
	
	auto end_time = std::chrono::high_resolution_clock::now();
	auto present_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - present_start);
	auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
	Logger::info("Frame presented in " + std::to_string(present_duration.count()) + "μs " +
		"(total render time: " + std::to_string(total_duration.count()) + "μs)");
}