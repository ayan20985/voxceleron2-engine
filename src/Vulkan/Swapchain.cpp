#include <algorithm>
#include <array>
#include "Swapchain.hpp"
#include "../Oreginum/Window.hpp"
#include "../Oreginum/Core.hpp"
#include "../Oreginum/LoggerMacros.hpp"

void Oreginum::Vulkan::Swapchain::initialize(const Instance& instance, std::shared_ptr<Surface> surface,
	std::shared_ptr<Device> device, const Command_Buffer& command_buffer)
{
	LOG_INFO("Initializing Vulkan swapchain");
	LOG_TIMER("Swapchain initialization");

	//Create swapchain
	LOG_DEBUG("Updating device surface capabilities");
	device->update();
	extent = device->get_surface_capabilities().currentExtent;

	LOG_DEBUG("Swapchain extent: " + std::to_string(extent.width) + "x" + std::to_string(extent.height));
	LOG_DEBUG("Min image count: " + std::to_string(device->get_surface_capabilities().minImageCount));
	LOG_DEBUG("Max image count: " + std::to_string(device->get_surface_capabilities().maxImageCount));
	LOG_DEBUG("Current transform: " + std::to_string(static_cast<int>(device->get_surface_capabilities().currentTransform)));

	vk::SwapchainKHR old_swapchain{*swapchain};
	LOG_DEBUG("Creating swapchain with format: " + std::to_string(static_cast<int>(Image::SWAPCHAIN_FORMAT)));
	LOG_DEBUG("Color space: " + std::to_string(static_cast<int>(Image::SWAPCHAIN_COLOR_SPACE)));
	vk::SwapchainCreateInfoKHR swapchain_information{{}, surface->get(),
		device->get_surface_capabilities().minImageCount, Image::SWAPCHAIN_FORMAT,
		Image::SWAPCHAIN_COLOR_SPACE, extent, 1, vk::ImageUsageFlagBits::eColorAttachment|
		vk::ImageUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive, 0, nullptr,
		device->get_surface_capabilities().currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::PresentModeKHR::eFifo, VK_TRUE, old_swapchain};

	std::array<uint32_t, 2> queue_indices{device->get_graphics_queue_family_index(),
		device->get_present_queue_family_index()};
	if(queue_indices[0] != queue_indices[1])
	{
		swapchain_information.setImageSharingMode(vk::SharingMode::eConcurrent);
		swapchain_information.setQueueFamilyIndexCount(2);
		swapchain_information.setPQueueFamilyIndices(queue_indices.data());
		LOG_DEBUG("Using concurrent sharing mode for graphics and present queues");
	}
	else
	{
		LOG_DEBUG("Using exclusive sharing mode (shared graphics/present queue)");
	}

	vk::Result result = device->get().createSwapchainKHR(&swapchain_information, nullptr, swapchain.get());
	VK_CHECK(result, "create swapchain");

	if(old_swapchain)
	{
		device->get().destroySwapchainKHR(old_swapchain);
		LOG_DEBUG("Destroyed old swapchain");
	}

	//Create image views
	images.clear();
	std::vector<vk::Image> image_handles{device->get().getSwapchainImagesKHR(*swapchain).value};

	LOG_INFO("Created swapchain with " + std::to_string(image_handles.size()) + " images");

	for(auto i : image_handles)
	{
		images.push_back({device, i});
		images.back().transition(command_buffer, vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR, vk::AccessFlags{}, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	}

	LOG_INFO("Swapchain initialized successfully");
}

Oreginum::Vulkan::Swapchain::~Swapchain()
{
	if(swapchain.use_count() != 1 || !*swapchain) return;
	device->get().destroySwapchainKHR(*swapchain);
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