#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Buffer.hpp"

Oreginum::Vulkan::Buffer::Buffer(std::shared_ptr<const Device> device,
	const Command_Buffer& temporary_command_buffer, vk::BufferUsageFlags usage,
	size_t size, const void *data, size_t uniform_size) : device(device),
	temporary_command_buffer(&temporary_command_buffer), size(size),
	descriptor_type(uniform_size ? vk::DescriptorType::eUniformBufferDynamic :
		vk::DescriptorType::eUniformBuffer)
{
	Logger::info("Creating Vulkan buffer with size: " + std::to_string(size) +
		" bytes, usage flags: " + std::to_string(static_cast<uint32_t>(usage)));
	
	if(uniform_size) {
		Logger::info("Dynamic uniform buffer with uniform size: " + std::to_string(uniform_size) + " bytes");
	} else {
		Logger::info("Standard uniform buffer");
	}

	Logger::info("Creating staging buffer (host visible, coherent)");
	create_buffer(&stage, &stage_memory, size, vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	
	Logger::info("Creating device buffer (device local, transfer destination)");
	create_buffer(buffer.get(), &buffer_memory, size, usage |
		vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
	
	if(data) {
		Logger::info("Writing initial data to buffer");
		write(data, size);
	}
	
	descriptor_information.buffer = *buffer;
	descriptor_information.offset = 0;
	descriptor_information.range = uniform_size ? uniform_size : size;
	
	Logger::info("Buffer creation completed successfully");
}

Oreginum::Vulkan::Buffer::~Buffer()
{
	if(buffer.use_count() != 1 || !device) {
		Logger::info("Buffer destructor: shared buffer or invalid device, skipping cleanup");
		return;
	}
	
	Logger::info("Destroying Vulkan buffer and associated resources");
	
	if(*buffer) {
		Logger::info("Destroying device buffer");
		vkDestroyBuffer(device->get(), *buffer, nullptr);
	}
	if(buffer_memory) {
		Logger::info("Freeing device buffer memory");
		vkFreeMemory(device->get(), buffer_memory, nullptr);
	}
	if(stage) {
		Logger::info("Destroying staging buffer");
		vkDestroyBuffer(device->get(), stage, nullptr);
	}
	if(stage_memory) {
		Logger::info("Freeing staging buffer memory");
		vkFreeMemory(device->get(), stage_memory, nullptr);
	}
	
	Logger::info("Buffer cleanup completed");
}

void Oreginum::Vulkan::Buffer::swap(Buffer *other)
{
	std::swap(device, other->device);
	std::swap(temporary_command_buffer, other->temporary_command_buffer);
	std::swap(size, other->size);
	std::swap(buffer, other->buffer);
	std::swap(buffer_memory, other->buffer_memory);
	std::swap(stage, other->stage);
	std::swap(stage_memory, other->stage_memory);
	std::swap(descriptor_information, other->descriptor_information);
	std::swap(descriptor_type, other->descriptor_type);
}

void Oreginum::Vulkan::Buffer::create_buffer(vk::Buffer *buffer, vk::DeviceMemory *memory,
	size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memory_property_flags)
{
	Logger::info("Creating buffer: size " + std::to_string(size) + " bytes, usage " +
		std::to_string(static_cast<uint32_t>(usage)) + ", memory properties " +
		std::to_string(static_cast<uint32_t>(memory_property_flags)));

	//Create buffer
	vk::BufferCreateInfo buffer_information{{}, size, usage};
	vk::Result result = device->get().createBuffer(&buffer_information, nullptr, buffer);
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create Vulkan buffer: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan buffer.");
	}
	Logger::info("Buffer created successfully");

	//Allocate buffer memory
	vk::MemoryRequirements memory_requirements
	(device->get().getBufferMemoryRequirements(*buffer));
	
	Logger::info("Memory requirements: size " + std::to_string(memory_requirements.size) +
		" bytes, alignment " + std::to_string(memory_requirements.alignment) +
		", type bits " + std::to_string(memory_requirements.memoryTypeBits));

	uint32_t memory_type_index = find_memory(*device, memory_requirements.memoryTypeBits, memory_property_flags);
	vk::MemoryAllocateInfo memory_information{memory_requirements.size, memory_type_index};
	
	result = device->get().allocateMemory(&memory_information, nullptr, memory);
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to allocate buffer memory: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not allocate memory for a Vulkan buffer.");
	}
	Logger::info("Buffer memory allocated: " + std::to_string(memory_requirements.size) +
		" bytes, memory type " + std::to_string(memory_type_index));

	//Bind buffer memory
	result = device->get().bindBufferMemory(*buffer, *memory, 0);
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to bind buffer memory: VkResult " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not bind memory to a Vulkan buffer.");
	}
	Logger::info("Buffer memory bound successfully");
}

uint32_t Oreginum::Vulkan::Buffer::find_memory(const Device& device,
	uint32_t type, vk::MemoryPropertyFlags properties)
{
	Logger::info("Finding suitable memory type for type bits " + std::to_string(type) +
		", required properties " + std::to_string(static_cast<uint32_t>(properties)));

	vk::PhysicalDeviceMemoryProperties memory_properties(device.get_gpu().getMemoryProperties());
	Logger::info("Device has " + std::to_string(memory_properties.memoryTypeCount) + " memory types");

	for(uint32_t i{}; i < memory_properties.memoryTypeCount; ++i)
		if((type & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags &
			properties) == properties) {
			Logger::info("Found suitable memory type: " + std::to_string(i));
			return i;
		}
	
	Logger::excep("Could not find suitable Vulkan memory type");
	Oreginum::Core::error("Could not find suitable Vulkan memory.");
	return NULL;
}

void Oreginum::Vulkan::Buffer::write(const void *data, size_t size, size_t offset)
{
	Logger::info("Writing " + std::to_string(size) + " bytes to buffer at offset " + std::to_string(offset));

	//Copy data to stage buffer
	this->size = size;
	Logger::info("Mapping staging buffer memory");
	auto result{device->get().mapMemory(stage_memory, offset, size)};
	if(result.result != vk::Result::eSuccess) {
		Logger::excep("Failed to map staging buffer memory: VkResult " + std::to_string(static_cast<int>(result.result)));
		Oreginum::Core::error("Could not map Vulkan buffer stage memory.");
	}
	
	Logger::info("Copying data to staging buffer");
	std::memcpy(result.value, data, size);
	device->get().unmapMemory(stage_memory);
	Logger::info("Staging buffer unmapped");

	//Copy data from stage buffer to device buffer
	Logger::info("Copying data from staging buffer to device buffer via command buffer");
	temporary_command_buffer->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	temporary_command_buffer->get().copyBuffer(stage, *buffer, {{offset, offset, size}});
	temporary_command_buffer->end_and_submit();
	Logger::info("Buffer write operation completed");
}