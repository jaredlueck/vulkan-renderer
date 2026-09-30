#include "cube.h"
#include "device.h"
#include "utils.h"
#include "physicaldevice.h"
#include "commandbuffer.h"
#include "buffer.h"

Cube::Cube(Device* device) {
	vertexBuffer = device->createBuffer(sizeof(cubeVertices[0]) * 24, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	vertexBuffer->copyData((void*)&cubeVertices[0]);
	
	indexBuffer = device->createBuffer(sizeof(cubeIndices[0]) * 36, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	indexBuffer->copyData((void*)&cubeIndices[0]);
};

void Cube::draw(CommandBuffer* commandBuffer) {
	VkBuffer vertexBuffers[] = { vertexBuffer->getHandle()};
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer->getHandle(), 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer->getHandle(), indexBuffer->getHandle(), 0, VkIndexType::VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(commandBuffer->getHandle(), 36, 1, 0, 0, 0);
}