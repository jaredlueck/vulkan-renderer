#pragma once
#include <vulkan/vulkan.h>

class RenderPipeline;
class CommandBuffer;
class Framebuffer;
class Device;

class RenderPass {
public:
	RenderPass(Device* device, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);
	VkRenderPass getHandle();
	void begin(CommandBuffer* commandBuffer, Framebuffer* framebuffer, VkExtent2D extent, VkViewport viewport, VkRect2D scissor);
	void end(CommandBuffer* commandBuffer);
	void setPipeline(RenderPipeline* pipeline);
private:
	VkRenderPass renderPass;
	Device* device;
	RenderPipeline* graphicsPipeline;
};