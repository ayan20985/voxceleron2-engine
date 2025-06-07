#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Descriptor Pool.hpp"

Oreginum::Vulkan::Descriptor_Pool::Descriptor_Pool(std::shared_ptr<Device> device,
	const std::vector<std::pair<vk::DescriptorType, uint32_t>>& sizes) : device(device)
{
	Logger::info("Creating descriptor pool with " + std::to_string(sizes.size()) + " pool sizes");
	
	uint32_t descriptor_set_count{};
	std::vector<vk::DescriptorPoolSize> pool_sizes{sizes.size()};
	
	for(uint32_t i{}; i < sizes.size(); ++i)
	{
		descriptor_set_count += sizes[i].second;
		pool_sizes[i] = {sizes[i].first, sizes[i].second};
		
		Logger::info("Pool size " + std::to_string(i) + ": type=" +
			std::to_string(static_cast<uint32_t>(sizes[i].first)) +
			", descriptorCount=" + std::to_string(sizes[i].second));
	}
	
	Logger::info("Total descriptor sets in pool: " + std::to_string(descriptor_set_count));

	vk::DescriptorPoolCreateInfo descriptor_pool_information{{}, descriptor_set_count,
		static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), };

	vk::Result result = device->get().createDescriptorPool(&descriptor_pool_information,
		nullptr, descriptor_pool.get());
		
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create descriptor pool with " + std::to_string(descriptor_set_count) +
			" total descriptors, VkResult: " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan descriptor pool.");
	}
	
	Logger::info("Descriptor pool created successfully with " + std::to_string(descriptor_set_count) +
		" descriptor sets, handle: " + std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkDescriptorPool>(*descriptor_pool))));
}

Oreginum::Vulkan::Descriptor_Pool::~Descriptor_Pool()
{
	if(descriptor_pool.use_count() != 1 || !*descriptor_pool) {
		Logger::info("Descriptor pool not destroyed - either shared or already destroyed");
		return;
	}
	
	Logger::info("Destroying descriptor pool, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkDescriptorPool>(*descriptor_pool))));
	device->get().destroyDescriptorPool(*descriptor_pool);
	Logger::info("Descriptor pool destroyed successfully");
}

void Oreginum::Vulkan::Descriptor_Pool::swap(Descriptor_Pool *other)
{
	Logger::info("Swapping descriptor pools");
	std::swap(device, other->device);
	std::swap(descriptor_pool, other->descriptor_pool);
	Logger::info("Descriptor pool swap completed");
}
