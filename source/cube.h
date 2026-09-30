#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
};

constexpr Vertex cubeVertices[] =
{
    // Front (+Z) - Red
    {-0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f},
    { 0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f},
    { 0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f},
    {-0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f},

    // Back (-Z) - Green
    { 0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f, 1.0f, 0.0f},
    {-0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f, 1.0f, 0.0f},
    {-0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f, 1.0f, 0.0f},
    { 0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f, 1.0f, 0.0f},

    // Left (-X) - Blue
    {-0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f},
    {-0.5f,-0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f},
    {-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f},
    {-0.5f, 0.5f,-0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f},

    // Right (+X) - Yellow
    { 0.5f,-0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f},
    { 0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f},
    { 0.5f, 0.5f,-0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f},
    { 0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f},

    // Bottom (-Y) - Magenta
    {-0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,  1.0f, 0.0f, 1.0f},
    { 0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,  1.0f, 0.0f, 1.0f},
    { 0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,  1.0f, 0.0f, 1.0f},
    {-0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,  1.0f, 0.0f, 1.0f},

    // Top (+Y) - Cyan
    {-0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f},
    {-0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f},
    { 0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f},
    { 0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f}
};

constexpr uint32_t cubeIndices[] =
{
    // Front
    0, 1, 2, 2, 3, 0,

    // Back
    4, 5, 6, 6, 7, 4,

    // Left
    8, 9, 10, 10, 11, 8,

    // Right
    12, 13, 14, 14, 15, 12,

    // Bottom
    16, 17, 18, 18, 19, 16,

    // Top
    20, 21, 22, 22, 23, 20
};
class Device;
class CommandBuffer;
class Buffer;

class Cube {
private:
	Buffer* vertexBuffer;
	Buffer* indexBuffer;
	glm::mat4x4 transform = glm::mat4x4(1.0f);
public:
    Cube(Device* device);
    void draw(CommandBuffer* commandBuffer);
};