#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 worldNormal;

layout(location = 0) out vec4 outColor;

layout (binding=0) uniform FrameData
{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightDir;
    vec4 cameraPos;
};

void main()
{
    vec3 n = normalize(worldNormal);
    vec3 l = normalize(-lightDir.xyz);
    vec3 v = normalize(cameraPos.xyz - worldPos.xyz);
    vec3 h = normalize(l + v);

    float ambientStrength = 0.1;
    float shininess = 32.0;

    vec3 ambient = ambientStrength * fragColor;

    float NdotL = max(dot(n, l), 0.0);
    vec3 diffuse = fragColor * NdotL;

    vec3 specular = vec3(0.0);

    if (NdotL > 0.0) {
        float NdotH = max(dot(n, h), 0.0);
        specular = vec3(1.0) * pow(NdotH, shininess);
    }

    outColor = vec4(ambient + diffuse + specular, 1.0);
}