#include "Core.hpp"
#include "Renderer Core.hpp"
#include "Texture.hpp"
#include "Logger.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Oreginum::Texture::Texture(const std::vector<std::string>& paths, const Vulkan::Sampler& sampler,
	Format type, bool cubemap) : type(type)
{
	std::string format_name = (type == Format::HDR) ? "HDR" :
							  (type == Format::RGB) ? "RGB" : "LINEAR";
	Logger::info("Creating texture with " + std::to_string(paths.size()) +
				" image(s), format: " + format_name +
				(cubemap ? " (cubemap)" : ""));
	
	std::vector<void *> datas;
	for(unsigned i{}; i < paths.size(); ++i)
	{
		Logger::info("Loading texture image: " + paths[i]);
		
		void *data;
		glm::ivec2 resolution;
		if(type == Format::HDR)
		{
			data = stbi_loadf(paths[i].c_str(), &resolution.x, &resolution.y, nullptr, STBI_rgb_alpha);
			Logger::info("Using HDR format for: " + paths[i]);
		}
		else
		{
			data = stbi_load(paths[i].c_str(), &resolution.x, &resolution.y, nullptr, STBI_rgb_alpha);
		}
		
		if(!data)
		{
			Logger::excep("Failed to load texture image: " + paths[i]);
			Core::error("Could not load image \""+paths[i]+"\".");
		}
		else
		{
			Logger::info("Successfully loaded texture: " + paths[i] + " (" +
						std::to_string(resolution.x) + "x" + std::to_string(resolution.y) + ")");
		}

		datas.push_back(data);
		if(i == 0)
		{
			this->resolution = resolution;
			Logger::info("Base texture resolution set: " + std::to_string(resolution.x) +
						"x" + std::to_string(resolution.y));
		}
		else if(this->resolution != resolution)
		{
			Logger::excep("Resolution mismatch in texture array - Expected: " +
						std::to_string(this->resolution.x) + "x" + std::to_string(this->resolution.y) +
						", Got: " + std::to_string(resolution.x) + "x" + std::to_string(resolution.y) +
						" for " + paths[i]);
			Core::error("Could not load image array because \""+paths[i]+"\" is a different resolution.");
		}
	}

	Logger::info("Creating Vulkan image with format: " + format_name);
	size_t total_memory = this->resolution.x * this->resolution.y * 4 * paths.size(); // 4 bytes per pixel (RGBA)
	Logger::info("Allocating texture memory: " + std::to_string(total_memory / 1024) + " KB");
	
	image = Vulkan::Image{Renderer_Core::get_device(), sampler,
		Renderer_Core::get_temporary_command_buffer(),
		this->resolution, datas, get_format(), cubemap};

	Logger::info("Texture creation completed successfully");
	
	for(void *d : datas) stbi_image_free(d);
}

vk::Format Oreginum::Texture::get_format()
{
	vk::Format format = (type == RGB) ? Vulkan::Image::RGB_FORMAT : (type == LINEAR) ?
		Vulkan::Image::LINEAR_FORMAT : Vulkan::Image::HDR_FORMAT_32;
	
	std::string format_name = (type == RGB) ? "RGB" :
							  (type == LINEAR) ? "LINEAR" : "HDR_32";
	Logger::info("Using Vulkan format: " + format_name);
	
	return format;
}