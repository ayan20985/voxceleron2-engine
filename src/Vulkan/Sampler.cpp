#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Sampler.hpp"

Oreginum::Vulkan::Sampler::Sampler(std::shared_ptr<Device> device, uint8_t lod, bool anisotropy,
	vk::SamplerAddressMode address_mode, vk::Filter close_filter,
	vk::Filter far_filter, vk::SamplerMipmapMode mipmap_mode) : device(device)
{
	Logger::info("Creating sampler with configuration:");
	Logger::info("  - Close filter: " + std::to_string(static_cast<uint32_t>(close_filter)));
	Logger::info("  - Far filter: " + std::to_string(static_cast<uint32_t>(far_filter)));
	Logger::info("  - Mipmap mode: " + std::to_string(static_cast<uint32_t>(mipmap_mode)));
	Logger::info("  - Address mode: " + std::to_string(static_cast<uint32_t>(address_mode)));
	Logger::info("  - LOD levels: " + std::to_string(lod));
	Logger::info("  - Anisotropy: " + std::string(anisotropy ? "enabled (16x)" : "disabled"));
	
	float max_anisotropy = anisotropy ? 16.0f : 1.0f;
	if(anisotropy) {
		Logger::info("Anisotropic filtering enabled with max samples: " + std::to_string(max_anisotropy));
	}
	
	vk::SamplerCreateInfo sampler_information;
	sampler_information.magFilter = close_filter;
	sampler_information.minFilter = far_filter;
	sampler_information.mipmapMode = mipmap_mode;
	sampler_information.addressModeU = address_mode;
	sampler_information.addressModeV = address_mode;
	sampler_information.addressModeW = address_mode;
	sampler_information.mipLodBias = 0.0f;
	sampler_information.anisotropyEnable = anisotropy;
	sampler_information.maxAnisotropy = max_anisotropy;
	sampler_information.compareEnable = VK_FALSE;
	sampler_information.compareOp = vk::CompareOp::eNever;
	sampler_information.minLod = 0.0f;
	sampler_information.maxLod = static_cast<float>(lod);
	sampler_information.borderColor = vk::BorderColor::eFloatOpaqueBlack;
	sampler_information.unnormalizedCoordinates = VK_FALSE;
		
	vk::Result result = device->get().createSampler(&sampler_information, nullptr, sampler.get());
	
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create sampler with LOD=" + std::to_string(lod) +
			", anisotropy=" + std::to_string(anisotropy ? max_anisotropy : 0.0f) +
			", VkResult: " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan sampler.");
	}
	
	Logger::info("Sampler created successfully, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkSampler>(*sampler))));
}

Oreginum::Vulkan::Sampler::~Sampler()
{
	if(sampler.use_count() != 1 || !*sampler || !device) {
		Logger::info("Sampler not destroyed - either shared, already destroyed, or device invalid");
		return;
	}
	
	Logger::info("Destroying sampler, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkSampler>(*sampler))));
	device->get().destroySampler(*sampler);
	Logger::info("Sampler destroyed successfully");
}

void Oreginum::Vulkan::Sampler::swap(Sampler *other)
{
	Logger::info("Swapping samplers");
	std::swap(device, other->device);
	std::swap(sampler, other->sampler);
	Logger::info("Sampler swap completed");
}