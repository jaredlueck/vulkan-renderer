#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Framebuffer;
class RenderPass;
class Device;
class PhysicalDevice;

class Swapchain {
public:
	Swapchain(Device* device, VkSurfaceKHR surface, uint32_t imageCount);
	Swapchain(Device* device, Swapchain* old, VkSurfaceKHR surface, uint32_t imageCount);
	~Swapchain();
	VkSwapchainKHR getHandle();
	VkResult acquireNextImage(uint32_t* imageIndex, VkSemaphore semaphore, VkFence fence);
	VkSurfaceFormatKHR getSurfaceFormat();
	std::vector<VkImageView> getImageViews() { return imageViews; }
	VkSurfaceCapabilitiesKHR getCapabilities() { return capabilities; }
	VkExtent2D getExtent();
	void createFramebuffers(RenderPass* renderPass);
	Framebuffer* getFramebuffer(size_t index);
private:
	Device* device;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	std::vector<VkImageView> imageViews;

	VkImage zBuffer;
	VkImageView zBufferView;
	
	std::vector<VkImage> images;
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<Framebuffer*> framebuffers;
};