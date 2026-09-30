#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Queue;
class PhysicalDevice;
class Buffer;

class Device {
public:
    VkDevice getHandle();
    Device(PhysicalDevice* physicalDevice);
    Queue* getGraphicsQueue(uint32_t queueIndex);
	VkSemaphore createSemaphore(VkSemaphoreCreateInfo* semaphoreInfo);
	PhysicalDevice* getPhysicalDevice();
    Buffer* createBuffer(VkDeviceSize size, VkBufferUsageFlags usage);

private:
    VkDevice device;
	PhysicalDevice* physicalDevice; 
};