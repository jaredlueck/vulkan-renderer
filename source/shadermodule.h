#pragma once
#include <vulkan/vulkan.h>

class Device;

class ShaderModule {
public:
	ShaderModule(Device* device, const char* shaderPath);
	VkShaderModule getHandle() { return shaderModule; }
private:
	VkShaderModule shaderModule;
};