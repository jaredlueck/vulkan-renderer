#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Queue;
class PhysicalDevice;
class Buffer;
class CommandBuffer;
class CommandPool;

class Device {
public:
    VkDevice getHandle();
    Device(PhysicalDevice* physicalDevice);
    Queue* getGraphicsQueue(uint32_t queueIndex);
	VkSemaphore createSemaphore(VkSemaphoreCreateInfo* semaphoreInfo);
	PhysicalDevice* getPhysicalDevice();
    Buffer* createBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
    uint32_t getMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
    void flushCommandBuffer(CommandBuffer* commandBuffer, Queue* queue, CommandPool* pool, bool free = true);
private:
    VkDevice device;
	PhysicalDevice* physicalDevice; 
    VkPhysicalDeviceMemoryProperties memoryProperties;
};