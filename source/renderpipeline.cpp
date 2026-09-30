#include "renderpipeline.h"
#include "utils.h"
#include <vector>
#include"device.h"
#include "renderpass.h"
#include "shadermodule.h"
#include "commandbuffer.h"
#include "framedata.h"
#include "buffer.h"

RenderPipeline::RenderPipeline(ShaderModule* vertexShader, ShaderModule* fragmentShader, 
	RenderPass* renderPass, Device* device) : device(device) {

	VkPushConstantRange pushConstant;
	//this push constant range starts at the beginning
	pushConstant.offset = 0;
	//this push constant range takes up the size of a MeshPushConstants struct
	pushConstant.size = sizeof(glm::mat4);
	//this push constant range is accessible only in the vertex shader
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// framedata descriptor set

	VkDescriptorSetLayoutBinding frameDataLayoutBinding
	{
		.binding = 0,
		.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL
	};
	
	VkDescriptorSetLayout frameDataLayout;

	VkDescriptorSetLayoutCreateInfo frameDataLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &frameDataLayoutBinding,
	};

	VkCall(vkCreateDescriptorSetLayout(device->getHandle(), &frameDataLayoutCreateInfo, nullptr, &frameDataLayout));

	// object data descriptor set

	VkDescriptorSetLayoutBinding objectDataLayoutBinding
	{
		.binding = 0,
		.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayout objectDataLayout;

	VkDescriptorSetLayoutCreateInfo objectDataLayoutCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &objectDataLayoutBinding,
	};

	vkCreateDescriptorSetLayout(device->getHandle(), &objectDataLayoutCreateInfo, nullptr, &objectDataLayout);

	VkDescriptorPoolSize poolSizes[2] = {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1
		},
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 3
		}
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 10,
		.poolSizeCount = 2,
		.pPoolSizes = &poolSizes[0]
	};

	VkDescriptorPool descriptorPool;
	VkCall(vkCreateDescriptorPool(device->getHandle(), &descriptorPoolCreateInfo, nullptr, &descriptorPool));

	VkDescriptorSetLayout layouts[2] = { frameDataLayout, objectDataLayout };
	
	VkDescriptorSetAllocateInfo frameDataDescriptorSetAllocateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &frameDataLayout
	};

	//descriptorSets.resize(2);

	VkCall(vkAllocateDescriptorSets(device->getHandle(), &frameDataDescriptorSetAllocateInfo, &framedataDescriptorSet));

	VkDescriptorSetLayout objectDataLayouts[3] = { objectDataLayout, objectDataLayout, objectDataLayout };

	VkDescriptorSetAllocateInfo objectDataDescriptorSetAllocateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 3,
		.pSetLayouts = &objectDataLayouts[0]
	};
	
	objectDataDescriptorSets.resize(3);
	VkCall(vkAllocateDescriptorSets(device->getHandle(), &objectDataDescriptorSetAllocateInfo, &objectDataDescriptorSets[0]));

	// Initialize shader stages
	VkPipelineShaderStageCreateInfo shaderStages[2] = {};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertexShader->getHandle();
	shaderStages[0].pName = "main";
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragmentShader->getHandle();
	shaderStages[1].pName = "main";

	VkVertexInputAttributeDescription attributes[5] = { 
		// position
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = 0,
		},
		// normal
		{
			.location = 1,
			.binding = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = 0,
		},
		// texcoord
		{
			.location = 2,
			.binding = 2,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = 0,
		},
		// weights
		{
			.location = 3,
			.binding = 3,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset = 0,
		},
		// joints
		{
			.location = 4,
			.binding = 4,
			.format = VK_FORMAT_R8G8B8A8_UINT,
			.offset = 0,
		},
	};

	VkVertexInputBindingDescription bindings[5] = {
		// position
		{
			.binding = 0,
			.stride = 3 * sizeof(float),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		},
		// normal
		{
			.binding = 1,
			.stride = 3 * sizeof(float),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
		// texcoord
		{
			.binding = 2,
			.stride = 2 * sizeof(float),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
		// weights
		{
			.binding = 3,
			.stride = 4 * sizeof(float),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
		// joints
		{
			.binding = 4,
			.stride = 4,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		}
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 5;
	vertexInputInfo.pVertexBindingDescriptions = &bindings[0];
	vertexInputInfo.vertexAttributeDescriptionCount = 5;
	vertexInputInfo.pVertexAttributeDescriptions = &attributes[0];

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = &layouts[0];
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

	VkCall(vkCreatePipelineLayout(device->getHandle(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	//viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	//viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS,
	};

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// Create the graphics pipeline
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicState;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	graphicsPipelineCreateInfo.pViewportState = &viewportState;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizer;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampling;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlending;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
	graphicsPipelineCreateInfo.layout = pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = renderPass->getHandle();
	VkCall(vkCreateGraphicsPipelines(device->getHandle(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline));

	uniformBuffer = device->createBuffer(sizeof(FrameData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	VkDescriptorBufferInfo bufferInfo
	{
		.buffer = uniformBuffer->getHandle(),
		.offset = 0,
		.range = VK_WHOLE_SIZE
	};

	VkWriteDescriptorSet writeDescriptorSet
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = framedataDescriptorSet,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &bufferInfo,
	};

 	vkUpdateDescriptorSets(device->getHandle(), 1, &writeDescriptorSet, 0, nullptr);
}

VkPipeline RenderPipeline::getHandle() { return graphicsPipeline; }

VkPipelineLayout RenderPipeline::getLayout() { return pipelineLayout; }

//VkDescriptorSet RenderPipeline::getDescriptorSet(int index) { return descriptorSets[index]; };

void RenderPipeline::setUniformData(void* data, uint32_t size) {
	uniformBuffer->copyData(data);
}

void RenderPipeline::bind(CommandBuffer* commandBuffer) {
	vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &framedataDescriptorSet, 0, nullptr);
	vkCmdBindPipeline(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}