#include "commandpool.h"
#include "utils.h"
#include "commandbuffer.h"
#include "device.h"

CommandPool::CommandPool(Device* device, uint32_t queueFamilyIndex) : device(device) {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkCall(vkCreateCommandPool(device->getHandle(), &poolInfo, nullptr, &commandPool));
}

CommandBuffer* CommandPool::allocateCommandBuffer() {
	return new CommandBuffer(device->getHandle(), commandPool);
}
