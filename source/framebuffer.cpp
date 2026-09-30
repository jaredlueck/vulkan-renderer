#include "framebuffer.h"
#include "utils.h"
#include "renderpass.h"
#include "device.h"

Framebuffer::Framebuffer(Device* device, RenderPass* renderPass, VkImageView* attachments, VkExtent2D extent) : device(device) {
    VkFramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = renderPass->getHandle();
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.attachmentCount = 2;
    framebufferCreateInfo.width = extent.width;
    framebufferCreateInfo.height = extent.height;
    framebufferCreateInfo.layers = 1;
    VkCall(vkCreateFramebuffer(device->getHandle(), &framebufferCreateInfo, nullptr, &this->framebuffer));
}

VkFramebuffer Framebuffer::getHandle() { return framebuffer; }