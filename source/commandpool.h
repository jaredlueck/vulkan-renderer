#pragma once
#include <vulkan/vulkan.h>

class CommandBuffer;
class Device;

class CommandPool {
public:
	CommandPool(Device* device, uint32_t queueFamilyIndex);
	CommandBuffer* allocateCommandBuffer();
private:
	VkCommandPool commandPool;
	Device* device;
};
