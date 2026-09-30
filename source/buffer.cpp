#include "buffer.h"
#include "device.h"
#include "utils.h"
#include "physicaldevice.h"

Buffer::Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage) {
	this->device = device->getHandle();
	this->size = size;
	VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage };

	VkCall(vkCreateBuffer(device->getHandle(), &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device->getHandle(), buffer, &memRequirements);

	PhysicalDevice* physicalDevice = device->getPhysicalDevice();

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice->getHandle(), &memProperties);

	uint32_t typeFilter = memRequirements.memoryTypeBits;

	uint32_t properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	uint32_t index = 0;

	for (int i = 0; i < memProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties)) {
			index = i;
		}
	}

	VkMemoryAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = index
	};

	VkCall(vkAllocateMemory(device->getHandle(), &allocInfo, nullptr, &memory));

	VkCall(vkBindBufferMemory(device->getHandle(), buffer, memory, 0));
	VkCall(vkMapMemory(device->getHandle(), memory, 0, size, 0, &memoryPtr));
}

void Buffer::copyData(void* data) {
	memcpy(memoryPtr, data, size);
}

VkBuffer Buffer::getHandle() {
	return buffer;
}

VkDeviceSize Buffer::getSize() {
	return size;
}