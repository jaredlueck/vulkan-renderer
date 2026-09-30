#pragma once
#include <vulkan/vulkan.h>

class Device;

class Buffer {
public:
	Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage);
	void copyData(void* data);
	VkBuffer getHandle();
	VkDeviceSize getSize();
private:
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDevice device;
	VkDeviceSize size;
	void* memoryPtr;
};