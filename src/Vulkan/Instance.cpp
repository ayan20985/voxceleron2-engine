#include <iostream>
#include "../Oreginum/Core.hpp"
#include "../Oreginum/Window.hpp"
#include "../Oreginum/Logger.hpp"
#include "Instance.hpp"

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback)
{
	auto fvkCreateDebugReportCallbackEXT{reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
		vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"))};
	return fvkCreateDebugReportCallbackEXT( instance, pCreateInfo, pAllocator, pCallback );
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance,
	VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator)
{
	auto fvkDestroyDebugReportCallbackEXT{reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"))};
	fvkDestroyDebugReportCallbackEXT(instance, callback, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_function(VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT type, uint64_t object, size_t location,
	int32_t code, const char *layer_prefix, const char *message, void *user_data)
{
 	std::cout<<layer_prefix<<": "<<message<<'\n';
	return false;
}

Oreginum::Vulkan::Instance::Instance(bool debug)
{
	Logger::info("Creating Vulkan instance with debug: " + std::string(debug ? "enabled" : "disabled"), true);
	
	vk::ApplicationInfo application_information
	{Oreginum::Window::get_title().c_str(), VK_MAKE_VERSION(0, 0, 1), "Oreginum Engine",
		VK_MAKE_VERSION(0, 0, 1), VK_API_VERSION_1_0};

	// Log base extensions
	Logger::info("Base Vulkan extensions: VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME");

	if(debug) {
		instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
		Logger::info("Added debug extension: VK_EXT_DEBUG_REPORT_EXTENSION_NAME");
		Logger::info("Added validation layer: VK_LAYER_LUNARG_standard_validation");
	}

	Logger::info("Total extensions: " + std::to_string(instance_extensions.size()) + ", layers: " + std::to_string(instance_layers.size()));

	vk::InstanceCreateInfo instance_information{{}, &application_information,
		static_cast<uint32_t>(instance_layers.size()), instance_layers.data(),
		static_cast<uint32_t>(instance_extensions.size()), instance_extensions.data()};

	vk::Result result = vk::createInstance(&instance_information, nullptr, instance.get());
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create Vulkan instance: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Vulkan is not supported sufficiently.");
	}

	Logger::info("Vulkan instance created successfully with " + std::to_string(instance_extensions.size()) + " extensions", true);

	if(debug) create_debug_callback();
}

Oreginum::Vulkan::Instance::~Instance()
{
	if(instance.use_count() != 1) {
		Logger::info("Vulkan instance destructor: shared instance, skipping cleanup");
		return;
	}
	
	Logger::info("Destroying Vulkan instance", true);
	
	if(debug_callback) {
		Logger::info("Destroying debug report callback");
		instance->destroyDebugReportCallbackEXT(debug_callback, nullptr);
	}
	
	if(*instance) {
		Logger::info("Destroying Vulkan instance handle");
		instance->destroy();
	}
	
	Logger::info("Vulkan instance cleanup completed");
}

void Oreginum::Vulkan::Instance::swap(Instance *other)
{
	std::swap(other->instance_extensions, this->instance_extensions);
	std::swap(other->instance_layers, this->instance_layers);
	std::swap(other->instance, this->instance);
	std::swap(other->debug_callback, this->debug_callback);
}

void Oreginum::Vulkan::Instance::create_debug_callback()
{
	Logger::info("Creating Vulkan debug report callback");
	
	vk::DebugReportCallbackCreateInfoEXT debug_callback_information;
	debug_callback_information.flags = vk::DebugReportFlagBitsEXT::eError |
		vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning;
	debug_callback_information.pfnCallback = reinterpret_cast<vk::PFN_DebugReportCallbackEXT>(debug_callback_function);

	Logger::info("Debug callback flags: Error | Warning | PerformanceWarning");

	vk::Result result = instance->createDebugReportCallbackEXT(&debug_callback_information,
		nullptr, &debug_callback);
	
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create debug report callback: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not initialize Vulkan debugging.");
	}
	
	Logger::info("Vulkan debug report callback created successfully");
}