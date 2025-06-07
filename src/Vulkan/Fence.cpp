#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Fence.hpp"

Oreginum::Vulkan::Fence::Fence(std::shared_ptr<Device> device, vk::FenceCreateFlags flags)
	: device(device)
{
	bool is_signaled = (flags & vk::FenceCreateFlagBits::eSignaled) != vk::FenceCreateFlags{};
	Logger::info("Creating fence with flags: " + std::to_string(static_cast<uint32_t>(flags)) +
		" (initial state: " + (is_signaled ? "signaled" : "unsignaled") + ")");
	
	vk::FenceCreateInfo fence_information{flags};
	vk::Result result = device->get().createFence(&fence_information, nullptr, fence.get());
	
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create fence, VkResult: " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan fence.");
	}
	
	Logger::info("Fence created successfully, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkFence>(*fence))) +
		", initial state: " + (is_signaled ? "signaled" : "unsignaled"));
}

Oreginum::Vulkan::Fence::~Fence()
{
	if(fence.use_count() != 1 || !*fence) {
		Logger::info("Fence not destroyed - either shared or already destroyed");
		return;
	}
	
	Logger::info("Destroying fence, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkFence>(*fence))));
	device->get().destroyFence(*fence);
	Logger::info("Fence destroyed successfully");
};

void Oreginum::Vulkan::Fence::swap(Fence *other)
{
	Logger::info("Swapping fences");
	std::swap(device, other->device);
	std::swap(fence, other->fence);
	Logger::info("Fence swap completed");
}