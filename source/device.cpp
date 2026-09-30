#include "device.h"
#include <vulkan/vulkan.h>
#include "queue.h"
#include "physicaldevice.h"
#include "utils.h"
#include "buffer.h"

Device::Device(PhysicalDevice* physicalDevice) : physicalDevice(physicalDevice) {
    const float queuePriority = 1.0f;
    // Create a logical device to interface with the physical device
    VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
    graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCreateInfo.pNext = nullptr;
    graphicsQueueCreateInfo.queueCount = 1;
    graphicsQueueCreateInfo.flags = 0;
    graphicsQueueCreateInfo.queueFamilyIndex = physicalDevice->getGraphicsQueueFamilyIndex();
    graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = physicalDevice->getFeatures();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{ graphicsQueueCreateInfo };
    VkDeviceCreateInfo deviceCreateInfo{};
    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;

	VkCall(vkCreateDevice(physicalDevice->getHandle(), &deviceCreateInfo, nullptr, &device));

}

VkDevice Device::getHandle() {
	return device;
}

Queue* Device::getGraphicsQueue(uint32_t queueIndex) {
	Queue* queue = new Queue(device, this->physicalDevice->getGraphicsQueueFamilyIndex(), queueIndex);
	return queue;
}

VkSemaphore Device::createSemaphore(VkSemaphoreCreateInfo* semaphoreInfo) {
	VkSemaphore semaphore;
	VkCall(vkCreateSemaphore(device, semaphoreInfo, nullptr, &semaphore));
	return semaphore;
}

PhysicalDevice* Device::getPhysicalDevice() {
	return physicalDevice;
}

Buffer* Device::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage) {
	Buffer* buffer = new Buffer(this, size, usage);
	return buffer;
}