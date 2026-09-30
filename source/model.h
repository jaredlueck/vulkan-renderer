#pragma once
#include <glm/glm.hpp>
#include "tinygltf/tiny_gltf_v3.h"
#include <vulkan/vulkan.hpp>
#include <glm/gtx/quaternion.hpp>

class Device;
class CommandBuffer;
class Buffer;
class RenderPipeline;

struct Primitive {
	uint64_t indexStart;
	uint32_t indexCount;
	uint64_t vertexStart;
	uint64_t normalStart;
	uint64_t texCoordStart;
	uint64_t jointsStart;
	uint64_t weightsStart;
};

struct Mesh {
	std::vector<Primitive> primitives;
};

struct Node
{
	Node* parent;
	uint32_t            index;
	std::vector<Node*> children;
	Mesh                mesh;
	glm::vec3           translation{};
	glm::vec3           scale{ 1.0f };
	glm::quat           rotation{};
	int32_t             skin = -1;
	glm::mat4           matrix;
	glm::mat4           getLocalMatrix();
	std::string name;
};

enum InterpolationType { LINEAR, STEP, CUBICSPLINE };

struct AnimationSampler
{
	std::string            interpolation;
	std::vector<float>     inputs;
	std::vector<glm::vec4> outputsVec4;
};

struct AnimationChannel
{
	std::string path;
	Node* node;
	uint32_t    samplerIndex;
};

struct Animation
{
	std::string                   name;
	std::vector<AnimationSampler> samplers;
	std::vector<AnimationChannel> channels;
	float                         start = std::numeric_limits<float>::max();
	float                         end = std::numeric_limits<float>::min();
	float                         currentTime = 0.0f;
};

struct Skin {
	uint32_t matrixStart;
	uint32_t matrixCount;
	std::vector<glm::mat4> inverseBindMatrices;
	std::vector<glm::mat4> jointMatrices;
	std::vector<Buffer*> ssbo;
	std::vector<VkDescriptorSet> descriptorSets;
	std::string            name;
	Node* skeletonRoot = nullptr;
	std::vector<Node*>    joints;
	VkDescriptorSet        descriptorSet;
};

class Model {
public:
	Model(Device* device, const char* path, RenderPipeline* pipeline);
	void setPipeline(RenderPipeline* pipeline);
	Device* mDevice;
	Buffer* mBuffer;
	Buffer* mMatrixBuffer;
	glm::mat4 getTransform();
	void updateAnimations(float deltaTime, uint32_t currentBuffer);
	void draw(CommandBuffer* commandBuffer);
private:
	void updateJoints(Node* node);
	void loadNode(tg3_node inputNode, tg3_model* data, Node* parent, uint32_t nodeIndex);
	void drawNode(CommandBuffer* commandBuffer, Node* node, glm::mat4 parentMatrix);
	void traverse(CommandBuffer* commandBuffer, Node* node, glm::mat4 parentMatrix);
	Node* getNodeFromIndex(int index);
	void loadAnimations(tg3_model* model);
	void loadSkins(tg3_model* model);
	void loadMaterials(tg3_model* model);
	glm::mat4 getNodeMatrix(Node* node);
	Node* findNode(Node* node, int index);
	Node* root;
	std::vector<Animation> animations;
	std::vector<Skin> skins;
	std::vector<Node*> nodes;
	RenderPipeline* pipeline;
	uint32_t currentBuffer;
};