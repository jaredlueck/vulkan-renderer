#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Device;
class RenderPass;

class Framebuffer {
public:
	Framebuffer(Device* device, RenderPass* renderPass, VkImageView* attachments, VkExtent2D extent);
	VkFramebuffer getHandle();
private:
	VkFramebuffer framebuffer;
	Device* device;
};