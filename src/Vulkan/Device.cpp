#include <iostream>
#include <map>
#include <set>
#include "../Oreginum/Core.hpp"
#include "../Oreginum/LoggerMacros.hpp"
#include "Swapchain.hpp"
#include "Device.hpp"

Oreginum::Vulkan::Device::Device(const Instance& instance, const Surface& surface)
	: instance(std::make_shared<const Instance>(instance)), surface(&surface)
{
	LOG_INFO("Creating Vulkan device");
	LOG_TIMER("Device initialization");

	select_gpu();
	create_device();

	LOG_INFO("Vulkan device created successfully");
}

Oreginum::Vulkan::Device::~Device()
{
	if(device.use_count() != 1 || !*device) return;
	LOG_DEBUG("Destroying Vulkan device");
	device->destroy();
	LOG_DEBUG("Vulkan device destroyed");
}

void Oreginum::Vulkan::Device::swap(Device *other)
{
	std::swap(instance, other->instance);
	std::swap(surface, other->surface);
	std::swap(gpu_extensions, other->gpu_extensions);
	std::swap(supported_gpu_extensions, other->supported_gpu_extensions);
	std::swap(graphics_queue_family_index, other->graphics_queue_family_index);
	std::swap(present_queue_family_index, other->present_queue_family_index);
	std::swap(graphics_queue, other->graphics_queue);
	std::swap(present_queue, other->present_queue);
	std::swap(gpu_properties, other->gpu_properties);
	std::swap(gpu_features, other->gpu_features);
	std::swap(surface_capabilities, other->surface_capabilities);
	std::swap(surface_formats, other->surface_formats);
	std::swap(swapchain_present_modes, other->swapchain_present_modes);
	std::swap(gpu, other->gpu);
	std::swap(device, other->device);
}

void Oreginum::Vulkan::Device::get_gpu_swapchain_information(const vk::PhysicalDevice& gpu)
{
	LOG_DEBUG("  Querying swapchain capabilities");
	gpu.getSurfaceCapabilitiesKHR(surface->get(), &surface_capabilities);
	surface_formats = gpu.getSurfaceFormatsKHR(surface->get()).value;
	swapchain_present_modes = gpu.getSurfacePresentModesKHR(surface->get()).value;
	LOG_DEBUG("  Surface formats: " + std::to_string(surface_formats.size()));
	LOG_DEBUG("  Present modes: " + std::to_string(swapchain_present_modes.size()));
}

void Oreginum::Vulkan::Device::get_gpu_information(const vk::PhysicalDevice& gpu)
{
		//General
		gpu.getProperties(&gpu_properties);
		gpu.getFeatures(&gpu_features);

		LOG_DEBUG("  GPU API version: " + std::to_string(gpu_properties.apiVersion >> 22) + "." +
			std::to_string((gpu_properties.apiVersion >> 12) & 0x3ff) + "." +
			std::to_string(gpu_properties.apiVersion & 0xfff));
		LOG_DEBUG("  GPU driver version: " + std::to_string(gpu_properties.driverVersion));
		LOG_DEBUG("  GPU vendor ID: " + std::to_string(gpu_properties.vendorID));

		//Extensions
		supported_gpu_extensions = gpu.enumerateDeviceExtensionProperties().value;
		LOG_DEBUG("  Supported extensions: " + std::to_string(supported_gpu_extensions.size()));

	//Graphics and present queues
	std::vector<vk::QueueFamilyProperties> queue_family_properties
	{gpu.getQueueFamilyProperties()};

	LOG_DEBUG("  Queue families: " + std::to_string(queue_family_properties.size()));
	graphics_queue_family_index = UINT32_MAX, present_queue_family_index = UINT32_MAX;
	for(uint32_t i{}; i < queue_family_properties.size(); ++i)
	{
		if(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			graphics_queue_family_index = i;
			LOG_DEBUG("    Queue family " + std::to_string(i) + ": Graphics (queues: " +
				std::to_string(queue_family_properties[i].queueCount) + ")");
		}
		vk::Bool32 surface_supported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(static_cast<VkPhysicalDevice>(gpu),
			i, static_cast<VkSurfaceKHR>(surface->get()), &surface_supported);
		if(surface_supported)
		{
			present_queue_family_index = i;
			LOG_DEBUG("    Queue family " + std::to_string(i) + ": Present support");
		}
		if(graphics_queue_family_index == present_queue_family_index) break;
	}

	//Swapchain
	get_gpu_swapchain_information(gpu);
}

void Oreginum::Vulkan::Device::select_gpu()
{
	LOG_DEBUG("Enumerating available GPUs");

	std::vector<vk::PhysicalDevice> gpus{instance->get().enumeratePhysicalDevices().value};

	if (gpus.empty())
	{
		LOG_FATAL("No Vulkan-capable GPUs found");
		Oreginum::Core::error("No Vulkan-capable GPUs found.");
	}

	LOG_INFO("Found " + std::to_string(gpus.size()) + " GPU(s)");

	std::map<int, vk::PhysicalDevice> gpu_ratings;
	for(const auto& g : gpus)
	{
		uint32_t rating{};
		std::string gpu_name;

		get_gpu_information(g);
		gpu_name = std::string(gpu_properties.deviceName.data());

		LOG_DEBUG("Evaluating GPU: " + gpu_name);

		//GPU type
		if(gpu_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			rating += 2;
			LOG_DEBUG("  Discrete GPU - +2 rating");
		}
		else
		{
			LOG_DEBUG("  Integrated GPU - +0 rating");
		}

		//Extensions
		std::set<std::string> required_extensions(gpu_extensions.begin(), gpu_extensions.end());
		for(const auto& e : supported_gpu_extensions)
			required_extensions.erase(e.extensionName);
		if(!required_extensions.empty())
		{
			LOG_DEBUG("  Missing required extensions, skipping");
			continue;
		}

		//Graphics queue
		if(graphics_queue_family_index == UINT32_MAX ||
			present_queue_family_index == UINT32_MAX)
		{
			LOG_DEBUG("  Missing graphics or present queue, skipping");
			continue;
		}
		if(graphics_queue_family_index == present_queue_family_index)
		{
			rating += 1;
			LOG_DEBUG("  Shared graphics/present queue - +1 rating");
		}

		//Swapchain minimum image count
		if((surface_capabilities.maxImageCount > 0) && (Swapchain::MINIMUM_IMAGE_COUNT >
			surface_capabilities.maxImageCount))
		{
			LOG_DEBUG("  Insufficient swapchain image count, skipping");
			continue;
		}

		//Swapchain format
		bool swapchain_format_supported{};
		if(!((surface_formats.size() == 1) &&
			(surface_formats[0].format == vk::Format::eUndefined)))
		{
			for(const auto& f : surface_formats)
				if(f.format == Image::SWAPCHAIN_FORMAT && f.colorSpace ==
					Image::SWAPCHAIN_COLOR_SPACE) swapchain_format_supported = true;
			if(!swapchain_format_supported)
			{
				LOG_DEBUG("  Unsupported swapchain format, skipping");
				continue;
			}
		}

		//Swapchain present mode
		bool swapchain_present_mode_supported{};
		for(const auto& p : swapchain_present_modes)
			if(p == vk::PresentModeKHR::eMailbox) swapchain_present_mode_supported = true;
		if(!swapchain_present_mode_supported)
		{
			LOG_DEBUG("  Mailbox present mode not supported, skipping");
			continue;
		}

		//Depth format
		vk::FormatProperties properties(g.getFormatProperties(Image::DEPTH_FORMAT));
		if((properties.optimalTilingFeatures & Image::DEPTH_FEATURES)
			!= Image::DEPTH_FEATURES)
		{
			LOG_DEBUG("  Unsupported depth format, skipping");
			continue;
		}

		gpu_ratings.insert({rating, g});
		LOG_DEBUG("  GPU rating: " + std::to_string(rating));
	}

	if(gpu_ratings.empty())
	{
		LOG_FATAL("No GPU meets minimum Vulkan requirements");
		Oreginum::Core::error("Could not find a GPU that supports Vulkan sufficiently.");
	}

	gpu = gpu_ratings.rbegin()->second; // Get highest rated GPU
	get_gpu_information(gpu);

	LOG_INFO("Selected GPU: " + std::string(gpu_properties.deviceName.data()) +
			 " (Rating: " + std::to_string(gpu_ratings.rbegin()->first) + ")");
}

void Oreginum::Vulkan::Device::create_device()
{
	LOG_DEBUG("Creating Vulkan logical device");

	std::vector<vk::DeviceQueueCreateInfo> device_queue_informations;
	static constexpr float QUEUE_PRIORITY{1};
	std::set<uint32_t> unique_queues{graphics_queue_family_index, present_queue_family_index};
	for(uint32_t q : unique_queues) device_queue_informations.push_back(
		vk::DeviceQueueCreateInfo{{}, q, 1, &QUEUE_PRIORITY});

	vk::PhysicalDeviceFeatures features{};
	features.setSamplerAnisotropy(true);
	features.setShaderStorageImageMultisample(true);
	features.setSampleRateShading(true);

	LOG_DEBUG("Requested device features:");
	LOG_DEBUG("  Sampler anisotropy: enabled");
	LOG_DEBUG("  Shader storage image multisample: enabled");
	LOG_DEBUG("  Sample rate shading: enabled");

	std::vector<const char*> extension_names;
	for (const auto& ext : gpu_extensions) extension_names.push_back(ext);

	vk::DeviceCreateInfo device_information{{},
		static_cast<uint32_t>(device_queue_informations.size()),
		device_queue_informations.data(), 0, nullptr,
		static_cast<uint32_t>(gpu_extensions.size()), extension_names.data(), &features};

	LOG_DEBUG("Requested device extensions: " + std::to_string(gpu_extensions.size()));
	for (const auto& ext : gpu_extensions) LOG_DEBUG("  " + std::string(ext));

	vk::Result result = gpu.createDevice(&device_information, nullptr, device.get());
	VK_CHECK(result, "create logical device");

	if (!device)
	{
		LOG_FATAL("Failed to create Vulkan logical device");
		Oreginum::Core::error("Could not create a Vulkan device.");
	}

	graphics_queue = device->getQueue(graphics_queue_family_index, 0);
	present_queue = device->getQueue(present_queue_family_index, 0);

	LOG_INFO("Vulkan logical device created successfully");
	LOG_DEBUG("Graphics queue family: " + std::to_string(graphics_queue_family_index));
	LOG_DEBUG("Present queue family: " + std::to_string(present_queue_family_index));
	LOG_DEBUG("Queues retrieved successfully");
}