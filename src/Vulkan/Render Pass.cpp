#include "../Oreginum/Core.hpp"
#include "../Oreginum/Logger.hpp"
#include "Swapchain.hpp"
#include "Render Pass.hpp"

Oreginum::Vulkan::Render_Pass::Render_Pass(std::shared_ptr<Device> device,
	const std::vector<vk::AttachmentDescription>& attachments,
	const std::vector<vk::SubpassDescription>& subpasses,
	const std::vector<vk::SubpassDependency>& dependencies)
	: device(device)
{
	Logger::info("Creating render pass with " + std::to_string(attachments.size()) + " attachments, " +
		std::to_string(subpasses.size()) + " subpasses, " +
		std::to_string(dependencies.size()) + " dependencies");
		
	// Log attachment details
	for(size_t i = 0; i < attachments.size(); ++i) {
		const auto& attachment = attachments[i];
		Logger::info("Attachment " + std::to_string(i) + ": format=" +
			std::to_string(static_cast<uint32_t>(attachment.format)) +
			", samples=" + std::to_string(static_cast<uint32_t>(attachment.samples)) +
			", loadOp=" + std::to_string(static_cast<uint32_t>(attachment.loadOp)) +
			", storeOp=" + std::to_string(static_cast<uint32_t>(attachment.storeOp)));
	}
	
	// Log subpass dependencies
	for(size_t i = 0; i < dependencies.size(); ++i) {
		const auto& dep = dependencies[i];
		Logger::info("Dependency " + std::to_string(i) + ": src=" + std::to_string(dep.srcSubpass) +
			", dst=" + std::to_string(dep.dstSubpass) +
			", srcStage=" + std::to_string(static_cast<uint32_t>(dep.srcStageMask)) +
			", dstStage=" + std::to_string(static_cast<uint32_t>(dep.dstStageMask)));
	}
	
	vk::RenderPassCreateInfo render_pass_information{{},
		static_cast<uint32_t>(attachments.size()), attachments.data(),
		static_cast<uint32_t>(subpasses.size()), subpasses.data(),
		static_cast<uint32_t>(dependencies.size()), dependencies.data()};

	vk::Result result = device->get().createRenderPass(&render_pass_information, nullptr, render_pass.get());
	
	if(result != vk::Result::eSuccess) {
		Logger::excep("Failed to create render pass with " + std::to_string(attachments.size()) +
			" attachments, VkResult: " + std::to_string(static_cast<int>(result)));
		Oreginum::Core::error("Could not create a Vulkan render pass.");
	}
	
	Logger::info("Render pass created successfully, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkRenderPass>(*render_pass))));
}

Oreginum::Vulkan::Render_Pass::~Render_Pass()
{
	if(render_pass.use_count() != 1 || !*render_pass) {
		Logger::info("Render pass not destroyed - either shared or already destroyed");
		return;
	}
	
	Logger::info("Destroying render pass, handle: " +
		std::to_string(reinterpret_cast<uintptr_t>(static_cast<VkRenderPass>(*render_pass))));
	device->get().destroyRenderPass(*render_pass);
	Logger::info("Render pass destroyed successfully");
}

void Oreginum::Vulkan::Render_Pass::swap(Render_Pass *other)
{
	Logger::info("Swapping render passes");
	std::swap(device, other->device);
	std::swap(render_pass, other->render_pass);
	Logger::info("Render pass swap completed");
}