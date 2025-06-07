#include "../Oreginum/Core.hpp"
#include "../Oreginum/Window.hpp"
#include "../Oreginum/Logger.hpp"
#include "Surface.hpp"

Oreginum::Vulkan::Surface::Surface(std::shared_ptr<Instance> instance) : instance(instance)
{
	Logger::info("Creating Vulkan Win32 surface with window integration");
	
	vk::Win32SurfaceCreateInfoKHR surface_information
	{{}, Oreginum::Window::get_instance(), Oreginum::Window::get()};
	
	Logger::info("Surface create info: HINSTANCE and HWND configured for Win32");
	
	vk::Result result = instance->get().createWin32SurfaceKHR(&surface_information, nullptr, surface.get());
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create Vulkan Win32 surface: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan surface.");
	}
	
	Logger::info("Vulkan Win32 surface created successfully");
}

Oreginum::Vulkan::Surface::~Surface()
{
	if(surface.use_count() == 1 && *surface) {
		Logger::info("Destroying Vulkan surface");
		instance->get().destroySurfaceKHR(*surface);
		Logger::info("Vulkan surface cleanup completed");
	} else {
		Logger::info("Surface destructor: shared surface or invalid, skipping cleanup");
	}
}

void Oreginum::Vulkan::Surface::swap(Surface *other)
{
	std::swap(instance, other->instance);
	std::swap(surface, other->surface);
}