#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Command Pool.hpp"

Oreginum::Vulkan::Command_Pool::Command_Pool(std::shared_ptr<Device> device,
	 uint32_t queue_family_index, vk::CommandPoolCreateFlags flags) : device(device)
{
	Logger::info("Creating command pool for queue family " + std::to_string(queue_family_index) +
		" with flags: " + std::to_string(static_cast<uint32_t>(flags)));
	
	vk::CommandPoolCreateInfo pool_information{flags, queue_family_index};
	vk::Result result = device->get().createCommandPool(&pool_information, nullptr, command_pool.get());
	
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create command pool for queue family " +
			std::to_string(queue_family_index) + ", VkResult: " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan command pool.");
	}
	
	Logger::info("Command pool created successfully for queue family " + std::to_string(queue_family_index) +
		", handle: " + std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkCommandPool>(*command_pool))));
}

Oreginum::Vulkan::Command_Pool::~Command_Pool()
{
	if(command_pool.use_count() != 1 || !*command_pool) {
		Logger::info("Command pool not destroyed - either shared or already destroyed");
		return;
	}
	
	Logger::info("Destroying command pool, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkCommandPool>(*command_pool))));
	device->get().destroyCommandPool(*command_pool);
	Logger::info("Command pool destroyed successfully");
}

void Oreginum::Vulkan::Command_Pool::swap(Command_Pool *other)
{
	Logger::info("Swapping command pools");
	std::swap(device, other->device);
	std::swap(command_pool, other->command_pool);
	Logger::info("Command pool swap completed");
}
