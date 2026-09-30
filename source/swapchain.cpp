#include "swapchain.h"
#include "utils.h"
#include "framebuffer.h"
#include "physicaldevice.h"
#include "device.h"

Swapchain::Swapchain(Device* device, VkSurfaceKHR surface, uint32_t imageCount) : device(device), surface(surface) {
    PhysicalDevice* physicalDevice = device->getPhysicalDevice();
    VkCall(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->getHandle(), surface, &capabilities));

    uint32_t formatCount;
    VkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->getHandle(), surface, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    VkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->getHandle(), surface, &formatCount, surfaceFormats.data()));

    VkSwapchainCreateInfoKHR swapChainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapChainCreateInfo.pNext = nullptr;
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormats[0].format;
    swapChainCreateInfo.imageColorSpace = surfaceFormats[0].colorSpace;
    swapChainCreateInfo.imageExtent = capabilities.currentExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.preTransform = capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkCall(vkCreateSwapchainKHR(device->getHandle(), &swapChainCreateInfo, nullptr, &swapchain));
    
    uint32_t availableImages;
    VkCall(vkGetSwapchainImagesKHR(device->getHandle(), swapchain, &availableImages, nullptr));
    images.resize(availableImages);
    VkCall(vkGetSwapchainImagesKHR(device->getHandle(), swapchain, &availableImages, images.data()));
    imageViews.resize(images.size());
}

Swapchain::Swapchain(Device* device, Swapchain* old, VkSurfaceKHR surface, uint32_t imageCount) : device(device), surface(surface) {
    PhysicalDevice* physicalDevice = device->getPhysicalDevice();
    VkCall(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->getHandle(), surface, &capabilities));

    uint32_t formatCount;
    VkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->getHandle(), surface, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    VkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->getHandle(), surface, &formatCount, surfaceFormats.data()));

    VkSwapchainCreateInfoKHR swapChainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormats[0].format,
        .imageColorSpace = surfaceFormats[0].colorSpace,
        .imageExtent = capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = old->getHandle()
    };

    VkCall(vkCreateSwapchainKHR(device->getHandle(), &swapChainCreateInfo, nullptr, &swapchain));

    uint32_t availableImages;
    VkCall(vkGetSwapchainImagesKHR(device->getHandle(), swapchain, &availableImages, nullptr));
    images.resize(availableImages);
    VkCall(vkGetSwapchainImagesKHR(device->getHandle(), swapchain, &availableImages, images.data()));

    imageViews.resize(images.size());
}

void Swapchain::createFramebuffers(RenderPass* renderPass) {
	this->framebuffers.resize(images.size());
    VkImage zBuffer;
    VkImageView zBufferView;

    VkExtent3D extent{
    .width = capabilities.currentExtent.width,
    .height = capabilities.currentExtent.height,
    .depth = 1
    };

    PhysicalDevice* physicalDevice = device->getPhysicalDevice();

    
    VkImageCreateInfo zbufferImageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VkFormat::VK_FORMAT_D32_SFLOAT,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    };

    VkCall(vkCreateImage(device->getHandle(), &zbufferImageInfo, nullptr, &zBuffer));

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device->getHandle(), zBuffer, &imageMemoryRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice->getHandle(), &memProperties);

    uint32_t typeFilter = imageMemoryRequirements.memoryTypeBits;

    uint32_t properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uint32_t index = 0;

    for (int i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties)) {
            index = i;
        }
    }

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageMemoryRequirements.size,
        .memoryTypeIndex = index
    };

    VkDeviceMemory memory;

    VkCall(vkAllocateMemory(device->getHandle(), &allocInfo, nullptr, &memory));

    VkCall(vkBindImageMemory(device->getHandle(), zBuffer, memory, 0));

    VkImageViewCreateInfo zBufferViewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = zBuffer,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VkFormat::VK_FORMAT_D32_SFLOAT,
        .components{
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange{.
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkCall(vkCreateImageView(device->getHandle(), &zBufferViewInfo, nullptr, &zBufferView));

	for (size_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = getSurfaceFormat().format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkCall(vkCreateImageView(device->getHandle(), &createInfo, nullptr, &imageViews[i]));
		
        VkImageView attachments[2] = { imageViews[i], zBufferView};
        Framebuffer* fb = new Framebuffer(device, renderPass, &attachments[0], this->getExtent());
		this->framebuffers[i] = fb;
	}
}

VkSwapchainKHR Swapchain::getHandle() {
	return swapchain;
}

VkResult Swapchain::acquireNextImage(uint32_t* imageIndex, VkSemaphore semaphore, VkFence fence) {
	return vkAcquireNextImageKHR(device->getHandle(), swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, imageIndex);
}

VkSurfaceFormatKHR Swapchain::getSurfaceFormat() {
    PhysicalDevice* physicalDevice = device->getPhysicalDevice();
	uint32_t formatCount;
	VkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->getHandle(), surface, &formatCount, nullptr));
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	VkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->getHandle(), surface, &formatCount, surfaceFormats.data()));
	return surfaceFormats[0];
}

Swapchain::~Swapchain() {
	if (swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device->getHandle(), swapchain, nullptr);
	}
}

Framebuffer* Swapchain::getFramebuffer(size_t index) { return framebuffers[index]; }

VkExtent2D Swapchain::getExtent() {
    PhysicalDevice* physicalDevice = device->getPhysicalDevice();
    VkSurfaceCapabilitiesKHR capabilities;
    VkCall(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->getHandle(), surface, &capabilities));
    return capabilities.currentExtent;
}
