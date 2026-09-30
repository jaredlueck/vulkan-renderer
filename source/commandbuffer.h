#pragma once
#include <vulkan/vulkan.hpp>
#include "utils.h"

class CommandBuffer {
public:
	CommandBuffer(VkDevice device, VkCommandPool commandPool);
	VkCommandBuffer getHandle();
	void reset();
	void begin();
	void end();
private:
	VkCommandBuffer commandBuffer;
};