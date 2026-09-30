#pragma once
#include <vulkan/vulkan.h>

class CommandBuffer;

class Queue {
public:
	Queue(VkDevice device, uint32_t queueFamilyIndex, uint32_t index);
	void submit(CommandBuffer* commandBuffer, VkSemaphore* imageAvailableSemaphore, VkSemaphore* renderFinishedSemaphore, VkFence* inFlightFence);
	void present(VkSemaphore* renderFinishedSemaphore, VkSwapchainKHR swapchain, uint32_t imageIndex);
private:
	VkQueue queue;
};