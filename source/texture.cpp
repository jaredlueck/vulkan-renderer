#include "texture.h"
#include "stbimage/stb_image.h"
#include <iostream>
#include "buffer.h"
#include "device.h"
#include "utils.h"
#include "commandbuffer.h"

Texture::Texture(const char* path, Device* device, CommandBuffer* commandBuffer) : mDevice(device)
{
    stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        std::cout << "Failed to load texture file " << path << std::endl;
        return;
    }
    unsigned char* buffer = nullptr;
    VkDeviceSize   bufferSize = 0;
    bool           deleteBuffer = false;

    // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
    if (channels == 3)
    {
        bufferSize = width * height * 4;
        buffer = new unsigned char[bufferSize];
        unsigned char* rgba = buffer;
        unsigned char* rgb = pixels;
        for (size_t i = 0; i < width * height; ++i)
        {
            memcpy(rgba, rgb, sizeof(unsigned char) * 3);
            rgba += 4;
            rgb += 3;
        }
        deleteBuffer = true;
    }
    else
    {
        buffer = pixels;
        bufferSize = width * height * channels;
    }

    Buffer* imBuffer = device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    imBuffer->copyData((void*)pixels);

    VkImageCreateInfo imageCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkCall(vkCreateImage(device->getHandle(), &imageCreateInfo, nullptr, &image));

    VkMemoryRequirements imageMemRequirements;
    vkGetImageMemoryRequirements(device->getHandle(), image, &imageMemRequirements);
    uint32_t memoryTypeIndex = device->getMemoryType(imageMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageMemRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };
    VkCall(vkAllocateMemory(device->getHandle(), &allocateInfo, nullptr, &memory));
    VkCall(vkBindImageMemory(device->getHandle(), image, memory, 0));

    VkImageSubresourceRange subResourceRange
    {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };

    VkImageMemoryBarrier imageMemoryBarrier
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = image,
        .subresourceRange = subResourceRange
    };

    vkCmdPipelineBarrier(commandBuffer->getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    // only one mip level for now
    VkBufferImageCopy imageCopyRegion{
        .bufferOffset = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageExtent = {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .depth = 1
        }
    };

    vkCmdCopyBufferToImage(commandBuffer->getHandle(), imBuffer->getHandle(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

    //vkDestroyBuffer(device->getHandle(), imBuffer->getHandle(), nullptr);

    if (deleteBuffer)
    {
        delete[] buffer;
    }

    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer->getHandle(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    VkImageViewCreateInfo imageViewCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 },
    };

    VkCall(vkCreateImageView(device->getHandle(), &imageViewCreateInfo, nullptr, &imageView));
}

Texture::~Texture()
{
    vkFreeMemory(mDevice->getHandle(), memory, nullptr);
    vkDestroyImage(mDevice->getHandle(), image, nullptr);
    vkDestroyImageView(mDevice->getHandle(), imageView, nullptr);
}