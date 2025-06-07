#include <iostream>
#include <map>
#include <set>
#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Swapchain.hpp"
#include "Device.hpp"

Oreginum::Vulkan::Device::Device(const Instance& instance, const Surface& surface)
	: instance(std::make_shared<const Instance>(instance)), surface(&surface)
{
	Logger::info("Creating Vulkan device with instance and surface", true);
	select_gpu();
	create_device();
	Logger::info("Vulkan device creation completed successfully", true);
}

Oreginum::Vulkan::Device::~Device()
{
	if(device.use_count() != 1 || !*device) {
		Logger::info("Vulkan device destructor: shared device or invalid, skipping cleanup");
		return;
	}
	
	Logger::info("Destroying Vulkan logical device", true);
	device->destroy();
	Logger::info("Vulkan device cleanup completed");
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
	gpu.getSurfaceCapabilitiesKHR(surface->get(), &surface_capabilities);
	surface_formats = gpu.getSurfaceFormatsKHR(surface->get()).value;
	swapchain_present_modes = gpu.getSurfacePresentModesKHR(surface->get()).value;
}

void Oreginum::Vulkan::Device::get_gpu_information(const vk::PhysicalDevice& gpu)
{
	//General
	gpu.getProperties(&gpu_properties);
	gpu.getFeatures(&gpu_features);
	
	Logger::info("GPU: " + std::string(gpu_properties.deviceName.data()) +
		" (Type: " + std::to_string(static_cast<int>(gpu_properties.deviceType)) +
		", API: " + std::to_string(VK_VERSION_MAJOR(gpu_properties.apiVersion)) + "." +
		std::to_string(VK_VERSION_MINOR(gpu_properties.apiVersion)) + "." +
		std::to_string(VK_VERSION_PATCH(gpu_properties.apiVersion)) + ")", true);

	//Extensions
	supported_gpu_extensions = gpu.enumerateDeviceExtensionProperties().value;
	Logger::info("GPU supports " + std::to_string(supported_gpu_extensions.size()) + " device extensions");

	//Graphics and present queues
	std::vector<vk::QueueFamilyProperties> queue_family_properties
	{gpu.getQueueFamilyProperties()};
	
	Logger::info("GPU has " + std::to_string(queue_family_properties.size()) + " queue families");

	graphics_queue_family_index = UINT32_MAX, present_queue_family_index = UINT32_MAX;
	for(uint32_t i{}; i < queue_family_properties.size(); ++i)
	{
		if(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics)
			graphics_queue_family_index = i;
		vk::Bool32 surface_supported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(static_cast<VkPhysicalDevice>(gpu),
			i, static_cast<VkSurfaceKHR>(surface->get()), &surface_supported);
		if(surface_supported) present_queue_family_index = i;
		if(graphics_queue_family_index == present_queue_family_index) break;
	}
	
	if(graphics_queue_family_index != UINT32_MAX)
		Logger::info("Graphics queue family index: " + std::to_string(graphics_queue_family_index));
	if(present_queue_family_index != UINT32_MAX)
		Logger::info("Present queue family index: " + std::to_string(present_queue_family_index));

	//Swapchain
	get_gpu_swapchain_information(gpu);
}

void Oreginum::Vulkan::Device::select_gpu()
{
	Logger::info("Selecting physical device (GPU)");
	std::vector<vk::PhysicalDevice> gpus{instance->get().enumeratePhysicalDevices().value};
	Logger::info("Found " + std::to_string(gpus.size()) + " physical devices");

	std::map<int, vk::PhysicalDevice> gpu_ratings;
	for(const auto& g : gpus)
	{
		uint32_t rating{};
		get_gpu_information(g);

		Logger::info("Evaluating GPU: " + std::string(gpu_properties.deviceName.data()));

		//GPU type
		if(gpu_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			rating += 2;
			Logger::info("Discrete GPU bonus: +2 rating");
		}

		//Extensions
		std::set<std::string> required_extensions(gpu_extensions.begin(), gpu_extensions.end());
		for(const auto& e : supported_gpu_extensions)
			required_extensions.erase(e.extensionName);
		if(!required_extensions.empty()) {
			Logger::warn("GPU missing required extensions, skipping");
			continue;
		}
		Logger::info("All required extensions supported");

		//Graphics queue
		if(graphics_queue_family_index == UINT32_MAX ||
			present_queue_family_index == UINT32_MAX) {
			Logger::warn("GPU missing graphics or present queue family, skipping");
			continue;
		}
		if(graphics_queue_family_index == present_queue_family_index) {
			rating += 1;
			Logger::info("Unified graphics/present queue bonus: +1 rating");
		}

		//Swapchain minimum image count
		if((surface_capabilities.maxImageCount > 0) && (Swapchain::MINIMUM_IMAGE_COUNT >
			surface_capabilities.maxImageCount)) {
			Logger::warn("GPU swapchain image count insufficient, skipping");
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
			if(!swapchain_format_supported) {
				Logger::warn("GPU swapchain format not supported, skipping");
				continue;
			}
		}

		//Swapchain present mode
		bool swapchain_present_mode_supported{};
		for(const auto& p : swapchain_present_modes)
			if(p == vk::PresentModeKHR::eMailbox) swapchain_present_mode_supported = true;
		if(!swapchain_present_mode_supported) {
			Logger::warn("GPU mailbox present mode not supported, skipping");
			continue;
		}

		//Depth format
		vk::FormatProperties properties(g.getFormatProperties(Image::DEPTH_FORMAT));
		if((properties.optimalTilingFeatures & Image::DEPTH_FEATURES)
			!= Image::DEPTH_FEATURES) {
			Logger::warn("GPU depth format features insufficient, skipping");
			continue;
		}

		Logger::info("GPU suitable with rating: " + std::to_string(rating));
		gpu_ratings.insert({rating, g});
	}

	if(gpu_ratings.empty()) {
		Logger::excep("No suitable GPU found that supports Vulkan sufficiently");
		Oreginum::Core::error("Could not find a GPU that supports Vulkan sufficiently.");
	}
	
	gpu = gpu_ratings.rbegin()->second; // Select highest rated GPU
	get_gpu_information(gpu);
	Logger::info("Selected GPU: " + std::string(gpu_properties.deviceName.data()) +
		" with rating: " + std::to_string(gpu_ratings.rbegin()->first), true);
}

void Oreginum::Vulkan::Device::create_device()
{
	Logger::info("Creating logical Vulkan device");
	
	std::vector<vk::DeviceQueueCreateInfo> device_queue_informations;
	static constexpr float QUEUE_PRIORITY{1};
	std::set<uint32_t> unique_queues{graphics_queue_family_index, present_queue_family_index};
	
	Logger::info("Creating " + std::to_string(unique_queues.size()) + " unique queue families");
	for(uint32_t q : unique_queues) {
		Logger::info("Queue family " + std::to_string(q) + " with priority " + std::to_string(QUEUE_PRIORITY));
		device_queue_informations.push_back(
			vk::DeviceQueueCreateInfo{{}, q, 1, &QUEUE_PRIORITY});
	}

	vk::PhysicalDeviceFeatures features{};
	features.setSamplerAnisotropy(true);
	features.setShaderStorageImageMultisample(true);
	features.setSampleRateShading(true);
	Logger::info("Enabled device features: SamplerAnisotropy, ShaderStorageImageMultisample, SampleRateShading");
	
	vk::DeviceCreateInfo device_information{{},
		static_cast<uint32_t>(device_queue_informations.size()),
		device_queue_informations.data(), 0, nullptr,
		static_cast<uint32_t>(gpu_extensions.size()), gpu_extensions.data(), &features};

	Logger::info("Device extensions: " + std::to_string(gpu_extensions.size()));

	vk::Result result = gpu.createDevice(&device_information, nullptr, device.get());
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create logical device: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan device.");
	}

	Logger::info("Logical device created successfully", true);

	graphics_queue = device->getQueue(graphics_queue_family_index, 0);
	present_queue = device->getQueue(present_queue_family_index, 0);
	
	Logger::info("Retrieved graphics queue from family " + std::to_string(graphics_queue_family_index));
	Logger::info("Retrieved present queue from family " + std::to_string(present_queue_family_index));
}