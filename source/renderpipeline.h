#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class ShaderModule;
class RenderPass;
class Device;
class CommandBuffer;
class Buffer;

class RenderPipelineBuilder {

};

class RenderPipeline {
public:
	enum DescriptorSetIndex { FRAME_DATA = 0, OBJECT_DATA = 1 };
	RenderPipeline(ShaderModule* vertexShader, ShaderModule* fragmentShader, RenderPass* renderPass, Device* device);
	VkPipelineLayout getLayout();
	VkPipeline getHandle();
	VkDescriptorSet getDescriptorSet(int index);
	void setUniformData(void* data, uint32_t size);
	void bind(CommandBuffer* commandBuffer);
	VkDescriptorSet framedataDescriptorSet;
	std::vector<VkDescriptorSet> objectDataDescriptorSets;
private:
	VkPipeline graphicsPipeline{};
	VkPipelineLayout pipelineLayout{};
	VkRenderPass renderPass{};
	//std::vector<VkDescriptorSet> descriptorSets;
	Device* device;
	Buffer* uniformBuffer;
};