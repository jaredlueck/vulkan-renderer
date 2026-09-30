#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 weights;
layout(location = 4) in uvec4 joints;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 worldPos;
layout(location = 2) out vec3 worldNormal;


layout (set = 0, binding = 0) uniform FrameData
{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightDir;
    vec4 cameraPos;
};

// TODO: use uniform storage
layout (std430, set = 1, binding = 0) readonly buffer JointData 
{
    mat4 jointMatrices[];
};

layout (push_constant) uniform constants 
{
    mat4 matrix;
} pushConstants1;


void main()
{
	// Calculate skinned matrix from weights and joint indices of the current vertex
	mat4 skinMat = 
		weights.x * jointMatrices[int(joints.x)] +
		weights.y * jointMatrices[int(joints.y)] +
		weights.z * jointMatrices[int(joints.z)] +
		weights.w * jointMatrices[int(joints.w)];

    vec4 worldPosition = model * pushConstants1.matrix * vec4(aPos, 1.0);

    worldPos = worldPosition.xyz;
    gl_Position = projection * view * worldPosition;

    worldNormal = inverse(transpose(mat3(model))) * aNormal;

    fragColor = worldNormal;
}