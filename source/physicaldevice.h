#pragma once
#include <vulkan/vulkan.h>

class PhysicalDevice {
public:
	PhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
	VkPhysicalDevice getHandle() const {
		return physicalDevice;
	}
	uint32_t getGraphicsQueueFamilyIndex() const {
		return graphicsQueueFamilyIndex;
	}
	VkPhysicalDeviceFeatures getFeatures() const {
		return features;
	}
	VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkSurfaceKHR surface) const;
private:
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceProperties properties;
	uint32_t graphicsQueueFamilyIndex = 0;
};