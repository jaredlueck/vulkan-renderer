#pragma once
#include <vulkan/vulkan.h>

class Instance {
public:
	Instance();
	VkInstance getHandle();
private:
	VkInstance instance;
};