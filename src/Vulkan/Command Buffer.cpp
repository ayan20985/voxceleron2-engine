#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Command Buffer.hpp"

Oreginum::Vulkan::Command_Buffer::Command_Buffer(std::shared_ptr<Device> device,
	const Command_Pool& command_pool, vk::CommandBufferLevel level)
	: device(device), command_pool(std::make_shared<Command_Pool>(command_pool))
{
	Logger::info("Creating Vulkan command buffer with level: " +
		std::string(level == vk::CommandBufferLevel::ePrimary ? "PRIMARY" : "SECONDARY"));
	
    fence = {device};
	Logger::info("Command buffer fence created");

	vk::CommandBufferAllocateInfo command_buffer_information{command_pool.get(), level, 1};
	vk::Result alloc_result = device->get().allocateCommandBuffers(&command_buffer_information,
		command_buffer.get());
	
	if(alloc_result != vk::Result::eSuccess) {
		Logger::excep("Failed to allocate command buffer: VkResult " + std::to_string(static_cast<int>(alloc_result)));
		Oreginum::Core::error("Could not allocate a Vulkan command buffer.");
	}
	
	Logger::info("Command buffer allocated successfully");
}

Oreginum::Vulkan::Command_Buffer::~Command_Buffer()
{
	if(command_buffer.use_count() != 1 || !*command_buffer) {
		Logger::info("Command buffer destructor: shared buffer or invalid handle, skipping cleanup");
		return;
	}
	
	Logger::info("Freeing Vulkan command buffer");
	device->get().freeCommandBuffers(command_pool->get(), {*command_buffer});
	Logger::info("Command buffer freed successfully");
}

void Oreginum::Vulkan::Command_Buffer::swap(Command_Buffer *other)
{
	std::swap(device, other->device);
	std::swap(command_pool, other->command_pool);
	std::swap(command_buffer, other->command_buffer);
	std::swap(fence, other->fence);
}

void Oreginum::Vulkan::Command_Buffer::begin(vk::CommandBufferUsageFlagBits flags) const
{
	Logger::info("Beginning command buffer recording with usage flags: " + std::to_string(static_cast<uint32_t>(flags)));
    
    Logger::info("Waiting for command buffer fence before recording");
    device->get().waitForFences({fence.get()}, VK_TRUE, std::numeric_limits<uint64_t>::max());
    Logger::info("Fence wait completed");
    
	vk::CommandBufferBeginInfo command_buffer_begin_information{flags};
	vk::Result begin_result = command_buffer->begin(command_buffer_begin_information);
	
	if(begin_result != vk::Result::eSuccess) {
		Logger::excep("Failed to begin command buffer recording: VkResult " + std::to_string(static_cast<int>(begin_result)));
		Core::error("Could not begin command buffer recording.");
	}
	
	Logger::info("Command buffer recording started successfully");
}

void Oreginum::Vulkan::Command_Buffer::end() const
{
	Logger::info("Ending command buffer recording");
	vk::Result end_result = command_buffer->end();
	
	if(end_result != vk::Result::eSuccess) {
		Logger::excep("Failed to end command buffer recording: VkResult " + std::to_string(static_cast<int>(end_result)));
		Core::error("Could not record a Vulkan command buffer.");
	}
	
	Logger::info("Command buffer recording completed successfully");
}

void Oreginum::Vulkan::Command_Buffer::submit() const
{
	Logger::info("Submitting command buffer to graphics queue");
	
	vk::SubmitInfo submit_information{0, nullptr,
		nullptr, 1, command_buffer.get(), 0, nullptr};
    
    Logger::info("Resetting command buffer fence before submission");
    vk::Result reset_result = device->get().resetFences({fence.get()});
    if(reset_result != vk::Result::eSuccess) {
    	Logger::warn("Failed to reset fence: VkResult " + std::to_string(static_cast<int>(reset_result)));
    }
    
    Logger::info("Submitting to graphics queue with fence synchronization");
    vk::Result submit_result = device->get_graphics_queue().submit(submit_information, fence.get());
    
    if(submit_result != vk::Result::eSuccess) {
    	Logger::excep("Failed to submit command buffer: VkResult " + std::to_string(static_cast<int>(submit_result)));
    	Core::error("Could not submit command buffer to graphics queue.");
    }
    
    Logger::info("Command buffer submitted successfully");
}