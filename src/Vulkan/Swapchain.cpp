#include <algorithm>
#include <array>
#include "Swapchain.hpp"
#include "../Oreginum/Window.hpp"
#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"

void Oreginum::Vulkan::Swapchain::initialize(const Instance& instance, std::shared_ptr<Surface> surface,
	std::shared_ptr<Device> device, const Command_Buffer& command_buffer)
{
	Logger::info("Initializing Vulkan swapchain", true);
	
	//Create swapchain
	device->update();
	extent = device->get_surface_capabilities().currentExtent;
	
	Logger::info("Swapchain extent: " + std::to_string(extent.width) + "x" + std::to_string(extent.height));
	Logger::info("Swapchain format: " + std::to_string(static_cast<int>(Image::SWAPCHAIN_FORMAT)) +
		", color space: " + std::to_string(static_cast<int>(Image::SWAPCHAIN_COLOR_SPACE)));

	vk::SwapchainKHR old_swapchain{*swapchain};
	if(old_swapchain) {
		Logger::info("Recreating swapchain (old swapchain exists)", true);
	} else {
		Logger::info("Creating new swapchain", true);
	}
	
	uint32_t min_image_count = device->get_surface_capabilities().minImageCount;
	Logger::info("Minimum image count: " + std::to_string(min_image_count));
	
	vk::SwapchainCreateInfoKHR swapchain_information{{}, surface->get(),
		min_image_count, Image::SWAPCHAIN_FORMAT,
		Image::SWAPCHAIN_COLOR_SPACE, extent, 1, vk::ImageUsageFlagBits::eColorAttachment|
		vk::ImageUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive, 0, nullptr,
		device->get_surface_capabilities().currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::PresentModeKHR::eFifo, VK_TRUE, old_swapchain};

	std::array<uint32_t, 2> queue_indices{device->get_graphics_queue_family_index(),
		device->get_present_queue_family_index()};
	if(queue_indices[0] != queue_indices[1])
	{
		Logger::info("Using concurrent sharing mode for different queue families");
		swapchain_information.setImageSharingMode(vk::SharingMode::eConcurrent);
		swapchain_information.setQueueFamilyIndexCount(2);
		swapchain_information.setPQueueFamilyIndices(queue_indices.data());
	} else {
		Logger::info("Using exclusive sharing mode for same queue family");
	}

	Logger::info("Present mode: FIFO, Composite alpha: Opaque");

	vk::Result result = device->get().createSwapchainKHR(&swapchain_information, nullptr, swapchain.get());
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create Vulkan swapchain: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create Vulkan swapchain.");
	}
	
	if(old_swapchain) {
		Logger::info("Destroying old swapchain", true);
		device->get().destroySwapchainKHR(old_swapchain);
	}

	Logger::info("Swapchain created successfully", true);

	//Create image views
	images.clear();
	std::vector<vk::Image> image_handles{device->get().getSwapchainImagesKHR(*swapchain).value};
	Logger::info("Acquired " + std::to_string(image_handles.size()) + " swapchain images");
	
	for(size_t idx = 0; idx < image_handles.size(); ++idx)
	{
		Logger::info("Processing swapchain image " + std::to_string(idx));
		images.push_back({device, image_handles[idx]});
		images.back().transition(command_buffer, vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR, vk::AccessFlags{}, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	}
	
	Logger::info("Swapchain initialization completed with " + std::to_string(images.size()) + " images", true);
}

Oreginum::Vulkan::Swapchain::~Swapchain()
{
	if(swapchain.use_count() != 1 || !*swapchain) {
		Logger::info("Swapchain destructor: shared swapchain or invalid, skipping cleanup");
		return;
	}
	
	Logger::info("Destroying Vulkan swapchain", true);
	device->get().destroySwapchainKHR(*swapchain);
	Logger::info("Swapchain cleanup completed");
}

void Oreginum::Vulkan::Swapchain::swap(Swapchain *other)
{
	std::swap(device, other->device);
	std::swap(surface, other->surface);
	std::swap(instance, other->instance);
	std::swap(extent, other->extent);
	std::swap(swapchain, other->swapchain);
	std::swap(images, other->images);
}