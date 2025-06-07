#include <iostream>
#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Buffer.hpp"
#include "Image.hpp"

const vk::FormatFeatureFlags Oreginum::Vulkan::Image::DEPTH_FEATURES
{vk::FormatFeatureFlagBits::eDepthStencilAttachment};

Oreginum::Vulkan::Image::Image(std::shared_ptr<Device> device, const Sampler& sampler,
	const Command_Buffer& temporary_command_buffer, const glm::uvec2& resolution,
	const std::vector<void *>& datas, vk::Format format, bool cubemap)
	: device(device), resolution(resolution), aspect(vk::ImageAspectFlagBits::eColor)
{
	Logger::info("Creating Vulkan image: " + std::to_string(resolution.x) + "x" + std::to_string(resolution.y) +
		", format " + std::to_string(static_cast<int>(format)) +
		", layers " + std::to_string(datas.size()) +
		", cubemap " + (cubemap ? "true" : "false"));

	unsigned CHANNELS{format == MONOCHROME_FORMAT ? 1U : 4U};
	const bool HDR32{format == Vulkan::Image::HDR_FORMAT_32};
	const unsigned BYTES_PER_PIXEL{HDR32 ? CHANNELS*4 : CHANNELS};
	const uint32_t LAYERS{static_cast<uint32_t>(datas.size())};
	mip_levels = static_cast<uint8_t>(floor(log2(std::max(resolution.x, resolution.y))));
	const bool ARRAY_2D{datas.size() > 1 && !cubemap};

	Logger::info("Image properties: " + std::to_string(CHANNELS) + " channels, " +
		std::to_string(BYTES_PER_PIXEL) + " bytes per pixel, " +
		std::to_string(mip_levels) + " mip levels, " +
		(HDR32 ? "HDR32" : "LDR") + " format");

	Logger::info("Creating staging images for " + std::to_string(LAYERS) + " layers");
	std::vector<std::pair<vk::Image, vk::DeviceMemory>> stages;
	for(size_t layer_idx = 0; layer_idx < datas.size(); ++layer_idx)
	{
		const void *d = datas[layer_idx];
		Logger::info("Processing layer " + std::to_string(layer_idx));

		//Create stage image
		Logger::info("Creating staging image for layer " + std::to_string(layer_idx));
		vk::Image stage_image{create_image(*device, {resolution.x, resolution.y}, 1,
			vk::ImageUsageFlagBits::eTransferSrc, format, aspect, 1, vk::ImageTiling::eLinear)};
		vk::DeviceMemory stage_image_memory{create_and_bind_image_memory(
			*device, stage_image, vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent)};

		//Copy datas to stage image
		uint32_t size{resolution.x*resolution.y*BYTES_PER_PIXEL};
		Logger::info("Mapping staging image memory: " + std::to_string(size) + " bytes");
		auto result{device->get().mapMemory(stage_image_memory, 0, size)};
		if(result.result != vk::Result::eSuccess) {
			Logger::excep("Failed to map staging image memory: VkResult " + std::to_string(static_cast<int>(result.result)));
			Core::error("Could not map Vulkan image stage memory.");
		}

		vk::ImageSubresource image_subresource{aspect, 0, 0};
		vk::SubresourceLayout stage_image_layout{
			device->get().getImageSubresourceLayout(stage_image, image_subresource)};

		if(stage_image_layout.rowPitch == resolution.x*BYTES_PER_PIXEL)
			std::memcpy(result.value, d, size);
		else
		{
			unsigned char *data_bytes{reinterpret_cast<unsigned char *>(result.value)};
			for(uint32_t i{}; i < resolution.y; ++i)
			{
				if(HDR32) memcpy(&data_bytes[i*stage_image_layout.rowPitch],
					&reinterpret_cast<const float *>(d)[i*resolution.y*BYTES_PER_PIXEL],
					resolution.x*BYTES_PER_PIXEL);
				else
				{
					memcpy(&data_bytes[i*stage_image_layout.rowPitch],
						&reinterpret_cast<const unsigned char *>(d)[
						i*resolution.x*BYTES_PER_PIXEL], resolution.x*BYTES_PER_PIXEL);
				}
			}
		}

		Logger::info("Copying data to staging image for layer " + std::to_string(layer_idx));
		device->get().unmapMemory(stage_image_memory);
		stages.push_back({stage_image, stage_image_memory});
	}
	
	//Create image
	Logger::info("Creating main image with optimal tiling");
	image = create_image(*device, {resolution.x, resolution.y}, mip_levels,
		vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eSampled, format, vk::ImageAspectFlagBits::eColor,
		LAYERS, vk::ImageTiling::eOptimal, ARRAY_2D, cubemap);
	image_memory = create_and_bind_image_memory(*device, image);

	//Transition image to use as copy destination
	Logger::info("Transitioning main image to transfer destination layout");
	transition(temporary_command_buffer, image, aspect,
		vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferDstOptimal,
		vk::AccessFlagBits::eHostWrite, vk::AccessFlagBits::eTransferWrite,
		vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer,
		0, LAYERS, 0, 1);

	//Copy stage images to image
	Logger::info("Copying staging images to main image and generating mipmaps");
	for(uint32_t i{}; i < LAYERS; ++i)
	{
		Logger::info("Processing layer " + std::to_string(i) + " for mipmap generation");

		//Transition to use as copy source
		transition(temporary_command_buffer, stages[i].first, aspect,
			vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferSrcOptimal,
			vk::AccessFlagBits::eHostWrite, vk::AccessFlagBits::eTransferRead,
			vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer);

		//Copy
		copy_image(temporary_command_buffer, stages[i].first, image, resolution, i);

		//Prepare base level for mipmap blit
		transition(temporary_command_buffer, image, aspect,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
			vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead,
			vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
			i, 1, 0, 1);

		//Mipmap blit
		Logger::info("Generating " + std::to_string(mip_levels - 1) + " mipmap levels for layer " + std::to_string(i));
		for(uint32_t j{1}; j < mip_levels; ++j)
		{
			vk::ImageBlit image_blit;
			image_blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			image_blit.srcSubresource.baseArrayLayer = i;
			image_blit.srcSubresource.layerCount = 1;
			image_blit.srcSubresource.mipLevel = j-1;
			image_blit.srcOffsets[1].x = static_cast<int32_t>(resolution.x>>(j-1));
			image_blit.srcOffsets[1].y = static_cast<int32_t>(resolution.y>>(j-1));
			image_blit.srcOffsets[1].z = 1;

			image_blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			image_blit.dstSubresource.baseArrayLayer = i;
			image_blit.dstSubresource.layerCount = 1;
			image_blit.dstSubresource.mipLevel = j;
			image_blit.dstOffsets[1].x = static_cast<int32_t>(resolution.x>>j);
			image_blit.dstOffsets[1].y = static_cast<int32_t>(resolution.y>>j);
			image_blit.dstOffsets[1].z = 1;

			//Prepare destination level for mipmap blit
			transition(temporary_command_buffer, image, aspect,
				vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferDstOptimal,
				vk::AccessFlagBits::eHostWrite, vk::AccessFlagBits::eTransferWrite,
				vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer,
				i, 1, j, 1);

			//Blit
			temporary_command_buffer.begin();
			temporary_command_buffer.get().blitImage(image, vk::ImageLayout::eTransferSrcOptimal,
				image, vk::ImageLayout::eTransferDstOptimal, {image_blit}, vk::Filter::eLinear);
			temporary_command_buffer.end_and_submit();

			//Transition destination to use as next source
			transition(temporary_command_buffer, image, aspect,
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
				vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead,
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
				i, 1, j, 1);
		}
	}

	//Final transition
	Logger::info("Final transition to shader read-only layout");
	transition(temporary_command_buffer, image, aspect,
		vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderRead,
		vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
		0, LAYERS, 0, mip_levels);

	//Create image view
	vk::ImageViewType view_type = cubemap ? vk::ImageViewType::eCube :
		ARRAY_2D ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
	Logger::info("Creating image view with type: " + std::to_string(static_cast<int>(view_type)));
	create_image_view(format, aspect, LAYERS, mip_levels, view_type);
	
	   //Deallocate stage device memory
	   Logger::info("Cleaning up " + std::to_string(stages.size()) + " staging images");
	   for(int i{}; i < stages.size(); ++i)
	{
	       device->get().destroyImage(stages[i].first);
	       device->get().freeMemory(stages[i].second);
	   }

	descriptor_information = {sampler.get(), *image_view, vk::ImageLayout::eShaderReadOnlyOptimal};
	Logger::info("Vulkan image creation completed successfully");
}

Oreginum::Vulkan::Image::Image(std::shared_ptr<Device> device, const Sampler& sampler,
	const glm::uvec2& resolution, vk::ImageUsageFlags usage, vk::Format format,
	vk::ImageAspectFlags aspect, vk::SampleCountFlagBits samples)
	: device(device), resolution(resolution), aspect(aspect)
{
	Logger::info("Creating render target image: " + std::to_string(resolution.x) + "x" + std::to_string(resolution.y) +
		", format " + std::to_string(static_cast<int>(format)) +
		", usage " + std::to_string(static_cast<uint32_t>(usage)) +
		", samples " + std::to_string(static_cast<int>(samples)));

	image = create_image(*device, resolution, 1, usage, format, aspect,
		1, vk::ImageTiling::eOptimal, false, false, samples);
	image_memory = create_and_bind_image_memory(*device, image);
	create_image_view(format, aspect);

	descriptor_information = {sampler.get(), *image_view, vk::ImageLayout::eShaderReadOnlyOptimal};
	Logger::info("Render target image creation completed");
}

Oreginum::Vulkan::Image::~Image()
{
	if(image_view.use_count() != 1 || !device) {
		Logger::info("Image destructor: shared image view or invalid device, skipping cleanup");
		return;
	}
	
	Logger::info("Destroying Vulkan image and associated resources");
	
	if(*image_view) {
		Logger::info("Destroying image view");
		device->get().destroyImageView(*image_view);
	}
	if(image_memory) {
		Logger::info("Freeing image memory");
		device->get().freeMemory(image_memory);
	}

	//If it is a swapchain image, it is automatically destroyed with the swapchain
	if(image && !swapchain) {
		Logger::info("Destroying image handle");
		device->get().destroyImage(image);
	} else if(swapchain) {
		Logger::info("Swapchain image - destruction handled by swapchain");
	}
	
	Logger::info("Image cleanup completed");
}

void Oreginum::Vulkan::Image::swap(Image *other)
{
	std::swap(device, other->device);
	std::swap(image, other->image);
	std::swap(image_memory, other->image_memory);
	std::swap(image_view, other->image_view);
	std::swap(aspect, other->aspect);
	std::swap(resolution, other->resolution);
	std::swap(mip_levels, other->mip_levels);
	std::swap(descriptor_information, other->descriptor_information);
	std::swap(swapchain, other->swapchain);
}

void Oreginum::Vulkan::Image::transition(const Command_Buffer& command_buffer,
	const vk::Image& image, vk::ImageAspectFlags aspect, vk::ImageLayout old_layout,
	vk::ImageLayout new_layout, vk::AccessFlags source_access_flags,
	vk::AccessFlags destination_access_flags, vk::PipelineStageFlags source_stage,
	vk::PipelineStageFlags destination_stage, uint32_t base_layer,
	uint32_t layers, uint32_t base_level, uint32_t levels, uint32_t source_queue_family_index,
	uint32_t destination_family_queue_index, bool temporary) const
{
	vk::ImageMemoryBarrier image_memory_barrier{source_access_flags, destination_access_flags,
		old_layout, new_layout, source_queue_family_index, destination_family_queue_index,
		image, {aspect, base_level, levels, base_layer, layers}};
	if(temporary) command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	command_buffer.get().pipelineBarrier(source_stage, destination_stage,
		vk::DependencyFlagBits{}, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
	if(temporary) command_buffer.end_and_submit();
}

vk::Image Oreginum::Vulkan::Image::create_image(const Device& device, const glm::uvec2& resolution,
	uint32_t mip_levels, vk::ImageUsageFlags usage, vk::Format format,
	vk::ImageAspectFlags aspect, uint32_t layers, vk::ImageTiling tiling,
	bool array_2d, bool cubemap, vk::SampleCountFlagBits samples)
{
	Logger::info("Creating image: " + std::to_string(resolution.x) + "x" + std::to_string(resolution.y) +
		", mips " + std::to_string(mip_levels) + ", layers " + std::to_string(layers) +
		", tiling " + (tiling == vk::ImageTiling::eOptimal ? "optimal" : "linear") +
		", samples " + std::to_string(static_cast<int>(samples)));

	vk::Image image;
	vk::ImageCreateInfo image_information{cubemap ? vk::ImageCreateFlagBits::eCubeCompatible :
		array_2d ? vk::ImageCreateFlagBits::e2DArrayCompatibleKHR : vk::ImageCreateFlags{},
		vk::ImageType::e2D, format, {resolution.x, resolution.y, 1},
		mip_levels, layers, samples, tiling, usage, vk::SharingMode::eExclusive,
		0, nullptr, vk::ImageLayout::ePreinitialized};

	vk::Result result = device.get().createImage(&image_information, nullptr, &image);
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create Vulkan image: VkResult " + std::to_string(static_cast<int>(result)));
		Core::error("Could not create a Vulkan image.");
	}
	Logger::info("Image created successfully");
	return image;
}

vk::DeviceMemory Oreginum::Vulkan::Image::create_and_bind_image_memory(const Device& device,
	const vk::Image& image, vk::MemoryPropertyFlags flags)
{
	Logger::info("Allocating and binding image memory with flags " + std::to_string(static_cast<uint32_t>(flags)));

	vk::DeviceMemory image_memory;
	vk::MemoryRequirements memory_requirements(device.get().getImageMemoryRequirements(image));
	
	Logger::info("Image memory requirements: size " + std::to_string(memory_requirements.size) +
		" bytes, alignment " + std::to_string(memory_requirements.alignment) +
		", type bits " + std::to_string(memory_requirements.memoryTypeBits));

	uint32_t memory_type_index = Buffer::find_memory(device, memory_requirements.memoryTypeBits, flags);
	vk::MemoryAllocateInfo memory_information{memory_requirements.size, memory_type_index};
	
	vk::Result result = device.get().allocateMemory(&memory_information, nullptr, &image_memory);
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to allocate image memory: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not allocate memory for a Vulkan image.");
	}
	
	Logger::info("Image memory allocated: " + std::to_string(memory_requirements.size) +
		" bytes, memory type " + std::to_string(memory_type_index));

	result = device.get().bindImageMemory(image, image_memory, 0);
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to bind image memory: VkResult " + std::to_string(static_cast<int>(result)));
	}
	Logger::info("Image memory bound successfully");
	return image_memory;
}

void Oreginum::Vulkan::Image::copy_image(const Command_Buffer& temporary_command_buffer,
	const vk::Image& source, const vk::Image& destination, const glm::uvec2& resolution,
	uint32_t layer, uint32_t level, vk::ImageAspectFlags aspect)
{
	vk::ImageSubresourceLayers source_subresource{aspect, 0, 0, 1};
	vk::ImageSubresourceLayers destination_subresource{aspect, level, layer, 1};
	vk::ImageCopy region{source_subresource, {}, destination_subresource,
		{}, {resolution.x, resolution.y, 1}};
	temporary_command_buffer.begin();
	temporary_command_buffer.get().copyImage(source, vk::ImageLayout::eTransferSrcOptimal,
		destination, vk::ImageLayout::eTransferDstOptimal, region);
	temporary_command_buffer.end_and_submit();
}

void Oreginum::Vulkan::Image::create_image_view(vk::Format format, vk::ImageAspectFlags aspect,
	uint32_t layers, uint32_t levels, vk::ImageViewType view_type)
{
	Logger::info("Creating image view: format " + std::to_string(static_cast<int>(format)) +
		", aspect " + std::to_string(static_cast<uint32_t>(aspect)) +
		", layers " + std::to_string(layers) + ", levels " + std::to_string(levels) +
		", type " + std::to_string(static_cast<int>(view_type)));

	vk::ImageViewCreateInfo image_view_information
	{{}, image, view_type, format, {}, {aspect, 0, levels, 0, layers}};
	
	vk::Result result = device->get().createImageView(&image_view_information, nullptr, image_view.get());
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create image view: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan image view.");
	}
	Logger::info("Image view created successfully");
}