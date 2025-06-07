#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Semaphore.hpp"

Oreginum::Vulkan::Semaphore::Semaphore(std::shared_ptr<Device> device) : device(device)
{
	Logger::info("Creating binary semaphore for GPU synchronization");
	
	vk::SemaphoreCreateInfo semaphore_information{};
	vk::Result result = device->get().createSemaphore(&semaphore_information, nullptr, semaphore.get());
	
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create semaphore, VkResult: " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan semaphore.");
	}
	
	Logger::info("Semaphore created successfully, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkSemaphore>(*semaphore))));
}

Oreginum::Vulkan::Semaphore::~Semaphore()
{
	if(semaphore.use_count() != 1 || !*semaphore) {
		Logger::info("Semaphore not destroyed - either shared or already destroyed");
		return;
	}
	
	Logger::info("Destroying semaphore, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkSemaphore>(*semaphore))));
	device->get().destroySemaphore(*semaphore);
	Logger::info("Semaphore destroyed successfully");
}

void Oreginum::Vulkan::Semaphore::swap(Semaphore *other)
{
	Logger::info("Swapping semaphores");
	std::swap(device, other->device);
	std::swap(semaphore, other->semaphore);
	Logger::info("Semaphore swap completed");
}