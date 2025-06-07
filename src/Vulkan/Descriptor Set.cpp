#include <iostream>
#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Descriptor Set.hpp"

Oreginum::Vulkan::Descriptor_Set::Descriptor_Set(std::shared_ptr<Device> device,
	const Descriptor_Pool& pool,
	const std::vector<std::pair<vk::DescriptorType, vk::ShaderStageFlags>>& bindings)
	: device(device), type(type)
{
	Logger::info("Creating descriptor set layout with " + std::to_string(bindings.size()) + " bindings");
	
	//Create layout
	std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_bindings;
	for(uint32_t i{}; i < bindings.size(); ++i) {
		descriptor_set_layout_bindings.push_back({i, bindings[i].first, 1, bindings[i].second});
		Logger::info("Binding " + std::to_string(i) + ": type=" +
			std::to_string(static_cast<uint32_t>(bindings[i].first)) +
			", stages=" + std::to_string(static_cast<uint32_t>(bindings[i].second)));
	}

	vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_information
	{{}, static_cast<uint32_t>(descriptor_set_layout_bindings.size()),
		descriptor_set_layout_bindings.data()};
		
	vk::Result layout_result = device->get().createDescriptorSetLayout(&descriptor_set_layout_information,
		nullptr, layout.get());
		
	if(layout_result != vk::Result::eSuccess) {
		Logger::excep("Failed to create descriptor set layout with " + std::to_string(bindings.size()) +
			" bindings, VkResult: " + std::to_string(static_cast<int>(layout_result)));
		Oreginum::Core::error("Could not create a Vulkan descriptor set layout.");
	}
	
	Logger::info("Descriptor set layout created successfully, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkDescriptorSetLayout>(*layout))));

	//Allocate set
	Logger::info("Allocating descriptor set from pool, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkDescriptorPool>(pool.get()))));
		
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_information{pool.get(), 1, layout.get()};
	vk::Result alloc_result = device->get().allocateDescriptorSets(&descriptor_set_allocate_information, &descriptor_set);
	
	if(alloc_result != vk::Result::eSuccess) {
		Logger::excep("Failed to allocate descriptor set from pool, VkResult: " +
			std::to_string(static_cast<int>(alloc_result)));
		Oreginum::Core::error("Could not allocate a Vulkan descriptor set.");
	}
	
	Logger::info("Descriptor set allocated successfully, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkDescriptorSet>(descriptor_set))));
}

Oreginum::Vulkan::Descriptor_Set::~Descriptor_Set()
{
	if(layout.use_count() != 1 || !*layout) {
		Logger::info("Descriptor set layout not destroyed - either shared or already destroyed");
		return;
	}
	
	Logger::info("Destroying descriptor set layout, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkDescriptorSetLayout>(*layout))));
	device->get().destroyDescriptorSetLayout(*layout);
	Logger::info("Descriptor set layout destroyed successfully");
}

void Oreginum::Vulkan::Descriptor_Set::write(const std::vector<const Uniform *>& uniforms)
{
	Logger::info("Updating descriptor set with " + std::to_string(uniforms.size()) + " uniform bindings");
	
	std::vector<vk::WriteDescriptorSet> write_descriptor_sets;

	for(uint32_t i{}; i < uniforms.size(); ++i)
	{
		const auto& desc_info = uniforms[i]->get_descriptor_information();
		write_descriptor_sets.push_back({descriptor_set, i, 0, 1,
			desc_info.type, desc_info.image, desc_info.buffer});
			
		Logger::info("Binding " + std::to_string(i) + ": type=" +
			std::to_string(static_cast<uint32_t>(desc_info.type)) +
			", buffer=" + (desc_info.buffer ? "valid" : "null") +
			", image=" + (desc_info.image ? "valid" : "null"));
	}
	
	device->get().updateDescriptorSets(write_descriptor_sets, {});
	Logger::info("Descriptor set updated successfully with " + std::to_string(uniforms.size()) + " bindings");
}

void Oreginum::Vulkan::Descriptor_Set::swap(Descriptor_Set *other)
{
	Logger::info("Swapping descriptor sets");
	std::swap(device, other->device);
	std::swap(type, other->type);
	std::swap(descriptor_set, other->descriptor_set);
	std::swap(layout, other->layout);
	Logger::info("Descriptor set swap completed");
}