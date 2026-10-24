#include "model.h"
#include <iostream>
#include "device.h"
#include "buffer.h"
#include "commandbuffer.h"
#include <glm/gtc/type_ptr.hpp>
#include "renderpipeline.h"
#include "stbimage/stb_image.h"
#include "texture.h"
#include "Welder/weldmesh.h"

int Primitive::getNumFaces(const SMikkTSpaceContext* pContext)
{
    // TODO: support other topologies
    Primitive* primitive = static_cast<Primitive*>(pContext->m_pUserData);
    // assuming triangles mode where each consecutive set of 3 vertices forms a triangle
    return primitive->indexCount / 3;
}

int Primitive::getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
{
    // TODO: support quads
    return 3;
}

void Primitive::getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
{
    Primitive* primitive = static_cast<Primitive*>(pContext->m_pUserData);
    uint32_t vertexIndex = 3 * iFace + iVert;

    glm::vec3 vertexPos = primitive->vertices[vertexIndex].position;

    fvPosOut[0] = vertexPos[0];
    fvPosOut[1] = vertexPos[1];
    fvPosOut[2] = vertexPos[2];
}

void Primitive::getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
{
    Primitive* primitive = static_cast<Primitive*>(pContext->m_pUserData);
    uint32_t vertexIndex = 3 * iFace + iVert;

    glm::vec3 normal = primitive->vertices[vertexIndex].normal;

    fvNormOut[0] = normal[0];
    fvNormOut[1] = normal[1];
    fvNormOut[2] = normal[2];
}

void Primitive::getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
{
    Primitive* primitive = static_cast<Primitive*>(pContext->m_pUserData);
    // This is the index into the index buffer that in turn gives the index of the vertex
    uint32_t vertexIndex = 3 * iFace + iVert;

    glm::vec2 texCoord = primitive->vertices[vertexIndex].texCoord;

    fvTexcOut[0] = texCoord[0];
    fvTexcOut[1] = texCoord[1];
}

void Primitive::setTangent(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
{
    Primitive* primitive = static_cast<Primitive*>(pContext->m_pUserData);
    
    glm::vec4 tangent = glm::vec4(fvTangent[0], fvTangent[1], fvTangent[2], fSign);
    uint32_t vertexIndex = 3 * iFace + iVert;
    primitive->vertices[vertexIndex].tangent = tangent;
}

glm::mat4 Node::getLocalMatrix() 
{
    glm::mat4 rot = glm::mat4(rotation);
    return glm::translate(glm::mat4(1.0), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0), scale) * matrix;
}

Model::Model(Device* device, const char* path, RenderPipeline* pipeline, CommandBuffer* commandBuffer) 
    : mDevice(device) , 
      pipeline(pipeline),
      path(path)
{
    tg3_parse_options opts;
    tg3_error_stack errors;
    tg3_model model;

    tg3_parse_options_init(&opts);
    tg3_error_stack_init(&errors);

    tg3_error_code err = tg3_parse_file(&model, &errors, path, strlen(path), &opts);
    if (err != TG3_OK) {
        for (uint32_t i = 0; i < errors.count; i++) {
            fprintf(stderr, "[%d] %s\n", (int)errors.entries[i].severity,
                errors.entries[i].message ? errors.entries[i].message : "(null)");
        }
    }
    // TODO: handle multiple buffers
    tg3_buffer buffer  = model.buffers[0];
    mBuffer = mDevice->createBuffer(buffer.byte_length, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    mBuffer->copyData((void*)buffer.data.data);

    tg3_scene scene = model.scenes[0];

    //nodes.resize(scene.nodes_count);

    for (size_t i = 0; i < scene.nodes_count; i++)
    {
        tg3_node node = model.nodes[scene.nodes[i]];
        loadNode(node, &model, nullptr, 0);
    }
    
    loadTextures(&model, commandBuffer);
    loadMaterials(&model, commandBuffer);
    loadSkins(&model);
    loadAnimations(&model);
    updateJoints(nodes[0]);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

void Model::loadNode(tg3_node inputNode, tg3_model* data, Node* parent, uint32_t nodeIndex) 
{   
    Node* node = new Node{};

    node->matrix = glm::mat4(1.0);
    node->skin = inputNode.skin;
    node->index = nodeIndex;
    node->parent = parent;
    node->name = inputNode.name.data;

    if (!parent)
    {
        root = node;
    }

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#transformations
    // To compose the local transformation matrix, TRS properties MUST be converted to matrices and postmultiplied in the T * R * S order; 
    // first the scale is applied to the vertices, then the rotation, and then the translation.

    node->translation = glm::make_vec3(inputNode.translation);
    glm::quat q = glm::make_quat(inputNode.rotation);
    node->rotation = glm::mat4(q);
    node->scale = glm::make_vec3(inputNode.scale);

    if (inputNode.has_matrix) {
        node->matrix = glm::make_mat4x4(inputNode.matrix);
    }

    // Load node's children
    if (inputNode.children_count > 0) 
    {
        for (size_t i = 0; i < inputNode.children_count; i++) 
        {
            tg3_node child = data->nodes[inputNode.children[i]];
            loadNode(child, data, node, inputNode.children[i]);
        }
    }

    if (inputNode.mesh > -1) 
    {
        const tg3_mesh tg3Mesh = data->meshes[inputNode.mesh];
        // Iterate through all primitives of this node's mesh
        for (size_t i = 0; i < tg3Mesh.primitives_count; i++)
        {
            Primitive primitive{};
            tg3_primitive gltfPrimitive = tg3Mesh.primitives[i];
            uint32_t indices = gltfPrimitive.indices;
            tg3_accessor accessor = data->accessors[indices];

            // gltf accessor types:
            // 5120 BYTE
            // 5121 UNSIGNED_BYTE
            // 5122 SHORT
            // 5123 UNSIGNED_SHORT
            // 5125 UNSIGNED_INT
            // 5126 FLOAT

            // TODO: map these components types to types accepted by the pipeline

            primitive.indexCount = accessor.count;
           
            int32_t indexType = accessor.component_type;
            
            tg3_buffer_view view = data->buffer_views[accessor.buffer_view];
            tg3_buffer buffer = data->buffers[view.buffer];

            uint16_t* indexPtr = (uint16_t*)&buffer.data.data[view.byte_offset + accessor.byte_offset];

            primitive.indexStart = view.byte_offset + accessor.byte_offset;

            primitive.materialIndex = gltfPrimitive.material;

            for (int j = 0; j < gltfPrimitive.attributes_count; j++)
            {
                std::string attributeName = gltfPrimitive.attributes[j].key.data;
                uint32_t attributeVal = gltfPrimitive.attributes[j].value;
                tg3_accessor accessor = data->accessors[attributeVal];
                tg3_buffer_view view = data->buffer_views[accessor.buffer_view];

                if (attributeName == "POSITION")
                {
                    primitive.vertexStart = view.byte_offset + accessor.byte_offset;
                }
                else if (attributeName == "NORMAL")
                {
                    primitive.normalStart = view.byte_offset + accessor.byte_offset;
                }
                else if (attributeName == "TEXCOORD_0")
                {
                    primitive.texCoordStart = view.byte_offset + accessor.byte_offset;
                }
                else if (attributeName == "JOINTS_0")
                {
                    primitive.jointsStart = view.byte_offset + accessor.byte_offset;
                }
                else if (attributeName == "WEIGHTS_0")
                {
                    primitive.weightsStart = view.byte_offset + accessor.byte_offset;
                }

                // TODO: support all modes
                if (gltfPrimitive.mode == 4)
                {

                }
            }

            // unpack the vertex attributes into an interleaved buffer
            primitive.vertices.resize(primitive.indexCount);

            glm::vec3* positonPtr = (glm::vec3*) &buffer.data.data[primitive.vertexStart];
            glm::vec3* normalPtr = (glm::vec3*) &buffer.data.data[primitive.normalStart];
            glm::vec2* texCoordPtr = (glm::vec2*) &buffer.data.data[primitive.texCoordStart];

            glm::vec4* weightsPtr = (glm::vec4*)&buffer.data.data[primitive.weightsStart];
            glm::u8vec4* jointsPtr = (glm::u8vec4*) &buffer.data.data[primitive.jointsStart];
            
            for (uint32_t i = 0; i < primitive.indexCount; i++)
            {
                uint16_t index = indexPtr[i];

                primitive.vertices[i].position = positonPtr[index];
                primitive.vertices[i].normal = normalPtr[index];
                primitive.vertices[i].texCoord = texCoordPtr[index];

                primitive.vertices[i].joints = jointsPtr[index];
                primitive.vertices[i].weights = weightsPtr[index];
            }

            SMikkTSpaceInterface interface
            {
                .m_getNumFaces = primitive.getNumFaces,
                .m_getNumVerticesOfFace = primitive.getNumVerticesOfFace,
                .m_getPosition = primitive.getPosition,
                .m_getNormal = primitive.getNormal,
                .m_getTexCoord = primitive.getTexCoord,
                .m_setTSpaceBasic = primitive.setTangent
            };

            SMikkTSpaceContext context
            {
                .m_pInterface = &interface,
                .m_pUserData = (void*)&primitive
            };
            
            genTangSpaceDefault(&context);

            // TODO: re-index primitive
            primitive.buffer = mDevice->createBuffer(primitive.vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            primitive.buffer->copyData(&primitive.vertices[0]);
            node->mesh.primitives.push_back(primitive);
        }
    }

    if (parent != nullptr)
    {
        parent->children.push_back(node);
    }
    else
    {
        nodes.push_back(node);
    }
}

void Model::draw(CommandBuffer* commandBuffer) 
{   
    this->traverse(commandBuffer, root, glm::mat4(1.0));
}

void Model::traverse(CommandBuffer* commandBuffer, Node* node, glm::mat4 parentMatrix) {
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#transformations
    // The global transformation matrix of a node is the product of the
    // global transformation matrix of its parent node and its own local transformation matrix.
    glm::mat4 matrix = node->matrix * parentMatrix;

    // if the node has a mesh, draw it
    if (node->mesh.primitives.size() > 0)
    {
        // TODO: remove
        if (node->name.compare("Object_36") != 0)
        {
            return;
        }
        this->drawNode(commandBuffer, node, matrix);
        return;
    }

    for (int i = 0; i < node->children.size(); i++)
    {
        Node* child = node->children[i];
        this->traverse(commandBuffer, child, matrix);
    }
}

void Model::drawNode(CommandBuffer* commandBuffer, Node* node, glm::mat4 transform){
   
    Mesh mesh = node->mesh;

    if (node->skin > -1)
    {
        Skin& skin = skins[node->skin];

        VkDescriptorSet descriptorSet = pipeline->objectDataDescriptorSets[currentBuffer];

        // Bind the descriptor set that references the shader storage buffer containing the joint matrices
        vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->getLayout(), 1, 1, &descriptorSet, 0, nullptr);
    }

    for (int i = 0; i < mesh.primitives.size(); i++) {
        Primitive primitive = mesh.primitives[i];

        VkBuffer vertexBuffer = primitive.buffer->getHandle();
        uint32_t vertexCount = primitive.vertices.size();
        
        // Update and bind the material for this primitive
        uint32_t materialIndex = primitive.materialIndex;
        Texture* baseColorTexture = textures[materials[materialIndex].baseColorTextureIndex];
        
        VkDescriptorImageInfo imageInfo
        {
            .sampler = sampler,
            .imageView = baseColorTexture->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet writeDescriptorSet
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = pipeline->diffuseTexDescriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo
        };

        vkUpdateDescriptorSets(mDevice->getHandle(), 1, &writeDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->getLayout(), 2, 1, &pipeline->diffuseTexDescriptorSet, 0, nullptr);

        // update and bind the normal map for this primitive

        glm::mat4 model = transform * getNodeMatrix(node);
        vkCmdPushConstants(commandBuffer->getHandle(), this->pipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform[0]);
        uint64_t offset = 0;
        vkCmdBindVertexBuffers(commandBuffer->getHandle(), 0, 1, &vertexBuffer, &offset);

        vkCmdDraw(commandBuffer->getHandle(), vertexCount, 1, 0, 0);
    }
}

glm::mat4 getNodeMatrix(Node* node)
{
    glm::mat4 nodeMatrix = node->getLocalMatrix();
    Node* parent = node->parent;
    while (parent)
    {
       nodeMatrix  = parent->getLocalMatrix() * nodeMatrix;
       parent = parent->parent;
    }

    return nodeMatrix;
}

void Model::updateJoints(Node* node)
{
    if (node->skin > -1)
    {
        Skin& skin = skins[node->skin];
        
        auto numJoints = (uint32_t)skin.joints.size();

        for (size_t index = 0; index < numJoints; index++)
        {
            // inverse of the nodes transform
            glm::mat4 inverseTransform = glm::inverse(getNodeMatrix(node));
            glm::mat4 inverseBindMatrix = skin.inverseBindMatrices[index];

            glm::mat4 jointMatrix = getNodeMatrix(skin.joints[index]);
            
            skin.jointMatrices[index] = jointMatrix * inverseBindMatrix;
            skin.jointMatrices[index] = inverseTransform * skin.jointMatrices[index];
        }

        skin.ssbo[currentBuffer]->copyData((void*)&skin.jointMatrices[0]);
    }

    for (auto& child : node->children)
    {
        updateJoints(child);
    }
}

glm::mat4 Model::getNodeMatrix(Node* node)
{
    glm::mat4 nodeMatrix = node->getLocalMatrix();
    Node* currentParent = node->parent;
    while (currentParent)
    {
        nodeMatrix = currentParent->getLocalMatrix() * nodeMatrix;
        currentParent = currentParent->parent;
    }
    return nodeMatrix;
}

void Model::updateAnimations(float deltaTime, uint32_t currentBuffer)
{
    this->currentBuffer = currentBuffer;
    // TODO: support multiple animations

    Animation& animation = animations[0];

    animation.currentTime = animation.currentTime + deltaTime;

    while (animation.currentTime > animation.end)
    {
        animation.currentTime -= animation.end;
    }

    for (auto& channel : animation.channels)
    {
        Node* node = channel.node;
        uint32_t samplerIndex = channel.samplerIndex;
        AnimationSampler& sampler = animation.samplers[samplerIndex];

        // find closest keyframe
        for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
        {
            if ((sampler.inputs[i] <= animation.currentTime) && (sampler.inputs[i + 1] >= animation.currentTime))
            {
                float a = (animation.currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);

                if (channel.path == "translation")
                {
                    node->translation = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
                }
                else if (channel.path == "rotation")
                {
                    glm::quat q1;
                    q1.x = sampler.outputsVec4[i].x;
                    q1.y = sampler.outputsVec4[i].y;
                    q1.z = sampler.outputsVec4[i].z;
                    q1.w = sampler.outputsVec4[i].w;

                    glm::quat q2;
                    q2.x = sampler.outputsVec4[i+1].x;
                    q2.y = sampler.outputsVec4[i+1].y;
                    q2.z = sampler.outputsVec4[i+1].z;
                    q2.w = sampler.outputsVec4[i+1].w;

                    node->rotation = glm::slerp(q1, q2, a);
                }
                else if (channel.path == "scale")
                {
                    node->scale = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
                }
            }
        }
    }

    for (auto& node : nodes)
    {
        updateJoints(node);
    }
}

void Model::loadSkins(tg3_model* input)
{
    skins.resize(input->skins_count);

    for (size_t i = 0; i < skins.size(); i++)
    {
        tg3_skin tg3Skin = input->skins[i];
        //skins[i].skeletonRoot = getNodeFromIndex(tg3Skin.skeleton);

        uint32_t jointCount = tg3Skin.joints_count;

        tg3_accessor accessor = input->accessors[tg3Skin.inverse_bind_matrices];
        tg3_buffer_view view = input->buffer_views[accessor.buffer_view];
        tg3_buffer buffer = input->buffers[view.buffer];

        void* dataPtr = (void*)&buffer.data.data[view.byte_offset + accessor.byte_offset];
        skins[i].inverseBindMatrices.resize(jointCount);
        memcpy(skins[i].inverseBindMatrices.data(), dataPtr, accessor.count * sizeof(glm::mat4));
        skins[i].inverseBindMatrices.resize(accessor.count);

        skins[i].jointMatrices.resize(jointCount);

        for (size_t j = 0; j < jointCount; j++)
        {
            Node* jointNode = getNodeFromIndex(tg3Skin.joints[j]);
            skins[i].joints.push_back(jointNode);
        }

        for (size_t j = 0; j < 3; j++)
        {
            Buffer* buffer = mDevice->createBuffer(accessor.count * sizeof(glm::mat4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            skins[i].ssbo.push_back(buffer);
        }

        for (size_t j = 0; j < 3; j++)
        {
            VkDescriptorBufferInfo bufferInfo
            {
                .buffer = skins[i].ssbo[j]->getHandle(),
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };

            VkWriteDescriptorSet writeDescriptorSet
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = pipeline->objectDataDescriptorSets[j],
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &bufferInfo,
            };

            vkUpdateDescriptorSets(mDevice->getHandle(), 1, &writeDescriptorSet, 0, nullptr);
        }
    }
}

void Model::loadAnimations(tg3_model* model) {
    skins.resize(model->skins_count);

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#skins
    Skin& skin = skins[0];
    
    animations.resize(model->animations_count);
    
    for (size_t i = 0; i < model->animations_count; i++)
    {
        tg3_animation animation = model->animations[i];
        animations[i].name = animation.name.data;
        
        uint32_t channelsCount = animation.channels_count;
        animations[i].channels.resize(channelsCount);

        for (size_t j = 0; j < channelsCount; j++) 
        {
            tg3_animation_channel tg3Channel = animation.channels[j];
            int32_t samplerIndex = tg3Channel.sampler;
            AnimationChannel& dstChannel = animations[i].channels[j];
            dstChannel.node = getNodeFromIndex(tg3Channel.target.node);
            dstChannel.path = tg3Channel.target.path.data;
            dstChannel.samplerIndex = tg3Channel.sampler;
        }

        uint32_t samplersCount = animation.samplers_count;
        animations[i].samplers.resize(samplersCount);

        for (int j = 0; j < samplersCount; j++)
        {
            tg3_animation_sampler sampler = animation.samplers[j];
            AnimationSampler& dstSampler = animations[i].samplers[j];
            {
                tg3_accessor accessor = model->accessors[sampler.input];
                tg3_buffer_view bufferView = model->buffer_views[accessor.buffer_view];
                tg3_buffer inputBuffer = model->buffers[bufferView.buffer];
                const void* dataPtr = &inputBuffer.data.data[bufferView.byte_offset + accessor.byte_offset];
                const float* buf = static_cast<const float*>(dataPtr);
                for (size_t i = 0; i < accessor.count; i++)
                {
                    dstSampler.inputs.push_back(buf[i]);
                }
                for (auto input : animations[i].samplers[j].inputs)
                {
                    if (input < animations[i].start)
                    {
                        animations[i].start = input;
                    }
                    if (input > animations[i].end)
                    {
                        animations[i].end = input;
                    }
                }
            }
            
            {
                tg3_accessor accessor = model->accessors[sampler.output];
                tg3_buffer_view bufferView = model->buffer_views[accessor.buffer_view];
                tg3_buffer buffer = model->buffers[bufferView.buffer];
                const void* dataPtr = &buffer.data.data[bufferView.byte_offset + accessor.byte_offset];
                for (size_t i = 0; i < accessor.count; i++)
                {
                    switch (accessor.type)
                    {
                        case TG3_TYPE_VEC3: {
                            const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
                            for (size_t index = 0; index < accessor.count; index++)
                            {
                                dstSampler.outputsVec4.push_back(glm::vec4(buf[index], 1.0));
                            }
                            break;
                        }
                        case TG3_TYPE_VEC4: {
                            const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
                            for (size_t index = 0; index < accessor.count; index++)
                            {
                                dstSampler.outputsVec4.push_back(buf[index]);
                            }
                            break;
                        }
                        default: {
                            std::cerr << "invalid type" << std::endl;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void Model::loadMaterials(tg3_model* model, CommandBuffer* commandBuffer)
{
    tg3_sampler tg3Sampler = model->samplers[0];
    VkSamplerCreateInfo samplerCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
    };

    vkCreateSampler(mDevice->getHandle(), &samplerCreateInfo, nullptr, &sampler);
    materials.resize(model->materials_count);

    for (size_t i = 0; i < model->materials_count; i++)
    {
        tg3_material mat = model->materials[i];
        tg3_pbr_metallic_roughness pbr = mat.pbr_metallic_roughness;
        tg3_texture_info baseColorTextureIndo = pbr.base_color_texture;

        tg3_normal_texture_info normalTextureInfo = mat.normal_texture;
        tg3_occlusion_texture_info occlusionTexture = mat.occlusion_texture;

        tg3_texture baseTexture = model->textures[baseColorTextureIndo.index];
        tg3_texture normalTexture = model->textures[normalTextureInfo.index];

        tg3_sampler sampler = model->samplers[baseTexture.sampler];

        //Texture base = textures[baseColorTextureIndo.index];
        //Texture normal = textures[normalTextureInfo.index];

        materials[i].baseColorTextureIndex = baseColorTextureIndo.index;
        materials[i].normalMapTextureIndex = normalTextureInfo.index;
    }
}

void Model::loadTextures(tg3_model* model, CommandBuffer* commandBuffer)
{
    for (size_t i = 0; i < model->textures_count; i++)
    {
        tg3_texture tg3Texture = model->textures[i];
        tg3_image image = model->images[tg3Texture.source];
        std::string imageRelativePath(image.uri.data);
        std::string fullPath = "./assets/dragon2/" + imageRelativePath;

        Texture* texture = new Texture(fullPath.c_str(), mDevice, commandBuffer);
        textures.push_back(texture);        
    }
}

Node* Model::findNode(Node* node, int index)
{
    if (node->index == index)
    {
        return node;
    }

    Node* found = nullptr;

    for (auto& child : node->children)
    {
        found = findNode(child, index);
        if (found)
        {
            break;
        }
    }
    return found;
}

Node* Model::getNodeFromIndex(int index)
{
    Node* found = nullptr;
    for (Node* node : nodes)
    {
        found = findNode(node, index);
        if (found)
        {
            break;
        }
    }
    return found;
}

void Model::setPipeline(RenderPipeline* pipeline) 
{
    this->pipeline = pipeline; 
}