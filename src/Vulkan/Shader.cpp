#include <fstream>
#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Shader.hpp"

Oreginum::Vulkan::Shader::Shader(std::shared_ptr<Device> device, const std::vector<
	std::pair<std::string, vk::ShaderStageFlagBits>>& shaders) : device(device)
{
	Logger::info("Creating shader with " + std::to_string(shaders.size()) + " shader stages");
	
	for(const auto& s : shaders)
	{
		Logger::info("Loading shader stage: " + s.first +
			", stage type: " + std::to_string(static_cast<uint32_t>(s.second)));
		
		modules->push_back(create_shader_module(s.first));
		information.push_back({{}, s.second, modules->back(), "main", nullptr});
		
		Logger::info("Shader stage " + s.first + " loaded successfully, module handle: " +
			std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkShaderModule>(modules->back()))));
	}
	
	Logger::info("Shader created with " + std::to_string(modules->size()) + " modules");
}

Oreginum::Vulkan::Shader::~Shader()
{
	if(modules.use_count() != 1) {
		Logger::info("Shader modules not destroyed - shared reference count: " +
			std::to_string(modules.use_count()));
		return;
	}
	
	Logger::info("Destroying " + std::to_string(modules->size()) + " shader modules");
	for(const vk::ShaderModule& m : *modules) {
		if(m) {
			Logger::info("Destroying shader module, handle: " +
				std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkShaderModule>(m))));
			device->get().destroyShaderModule(m);
		}
	}
	
	Logger::info("All shader modules destroyed successfully");
}

void Oreginum::Vulkan::Shader::swap(Shader *other)
{
	Logger::info("Swapping shader objects");
	std::swap(device, other->device);
	std::swap(modules, other->modules);
	std::swap(information, other->information);
	Logger::info("Shader swap completed");
}

vk::ShaderModule Oreginum::Vulkan::Shader::create_shader_module(const std::string& shader)
{
	std::string shader_path = "Resources/Shaders/" + shader + ".spv";
	Logger::info("Creating shader module from SPIR-V file: " + shader_path);
	
	std::ifstream file{shader_path, std::ios::ate | std::ios::binary};
	if(!file.is_open()) {
		Logger::excep("Failed to open shader file: " + shader_path);
		Oreginum::Core::error("Could not open shader \""+shader+"\".");
	}
	
	size_t size{static_cast<size_t>(file.tellg())};
	Logger::info("Shader file size: " + std::to_string(size) + " bytes");
	
	file.seekg(0);
	std::vector<char> data(size);
	file.read(data.data(), size);
	file.close();

	// Validate SPIR-V header
	if(size < 20 || *reinterpret_cast<const uint32_t*>(data.data()) != 0x07230203) {
		Logger::warn("Shader file may not be valid SPIR-V bytecode: " + shader);
	}

	vk::ShaderModuleCreateInfo shader_module_information;
	shader_module_information.setCodeSize(data.size());
	shader_module_information.setPCode(reinterpret_cast<const uint32_t *>(data.data()));

	vk::ShaderModule shader_module;
	vk::Result result = device->get().createShaderModule(&shader_module_information,
		nullptr, &shader_module);
		
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create shader module \"" + shader +
			"\", VkResult: " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create Vulkan shader module \""+shader+"\".");
	}
	
	Logger::info("Shader module \"" + shader + "\" created successfully, size: " +
		std::to_string(size) + " bytes, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkShaderModule>(shader_module))));
	
	return shader_module;
}