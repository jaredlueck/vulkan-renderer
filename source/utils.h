#pragma once
#include <vulkan/vulkan.h>
#include <iostream>

inline void VkCall(VkResult result) {
	if (result != VK_SUCCESS) {
		std::cerr << "Vulkan error: " << result << std::endl;
		exit(-1);
	}
}