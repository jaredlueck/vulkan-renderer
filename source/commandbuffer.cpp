#include "commandbuffer.h"
#include "utils.h"

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCall(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));
}

VkCommandBuffer CommandBuffer::getHandle() { return commandBuffer; }

void CommandBuffer::reset() {
	VkCall(vkResetCommandBuffer(commandBuffer, 0));
}

void CommandBuffer::begin() {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VkCall(vkBeginCommandBuffer(commandBuffer, &beginInfo));
}

void CommandBuffer::end() {
	VkCall(vkEndCommandBuffer(commandBuffer));
}
