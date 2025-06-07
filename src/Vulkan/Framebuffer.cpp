#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Framebuffer.hpp"

Oreginum::Vulkan::Framebuffer::Framebuffer(std::shared_ptr<Device> device,
	const glm::uvec2& resolution, const Render_Pass& render_pass,
	const std::vector<const Image *>& attachments)
    : device(device), resolution(resolution)
{
	Logger::info("Creating framebuffer with dimensions " + std::to_string(resolution.x) + "x" +
		std::to_string(resolution.y) + " and " + std::to_string(attachments.size()) + " attachments");
	
	std::vector<vk::ImageView> attachment_views;
	for(size_t i = 0; i < attachments.size(); ++i) {
		const Image* attachment = attachments[i];
		attachment_views.emplace_back(attachment->get_view());
		Logger::info("Attachment " + std::to_string(i) + " - Image view handle: " +
			std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkImageView>(attachment->get_view()))));
	}

	vk::FramebufferCreateInfo framebuffer_information{{}, render_pass.get(),
		static_cast<uint32_t>(attachment_views.size()), attachment_views.data(),
		resolution.x, resolution.y, 1};

	Logger::info("Framebuffer configuration - Render pass handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkRenderPass>(render_pass.get()))) +
		", layers: 1, render pass compatible");

	vk::Result result = device->get().createFramebuffer(&framebuffer_information,
		nullptr, framebuffer.get());
		
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create framebuffer " + std::to_string(resolution.x) + "x" +
			std::to_string(resolution.y) + " with " + std::to_string(attachments.size()) +
			" attachments, VkResult: " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan framebuffer.");
	}
	
	Logger::info("Framebuffer created successfully - " + std::to_string(resolution.x) + "x" +
		std::to_string(resolution.y) + ", handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkFramebuffer>(*framebuffer))));
}

Oreginum::Vulkan::Framebuffer::~Framebuffer()
{
	if(framebuffer.use_count() != 1 || !*framebuffer) {
		Logger::info("Framebuffer not destroyed - either shared or already destroyed");
		return;
	}
	
	Logger::info("Destroying framebuffer " + std::to_string(resolution.x) + "x" +
		std::to_string(resolution.y) + ", handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkFramebuffer>(*framebuffer))));
	device->get().destroyFramebuffer(*framebuffer);
	Logger::info("Framebuffer destroyed successfully");
}

void Oreginum::Vulkan::Framebuffer::swap(Framebuffer *other)
{
	Logger::info("Swapping framebuffers");
	std::swap(device, other->device);
	std::swap(framebuffer, other->framebuffer);
	std::swap(resolution, other->resolution);
	Logger::info("Framebuffer swap completed");
}