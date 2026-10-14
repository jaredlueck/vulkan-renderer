#pragma once
#include <vulkan/vulkan.h>

class Device;
class CommandBuffer;

class Texture 
{
private:
	VkImage image;
	VkDeviceMemory memory;
	Device* mDevice;
	int width;
	int height;
	int channels;
public:
	Texture(const char* path, Device* device, CommandBuffer* commandBuffer);
	~Texture();
	VkImageView imageView;
};