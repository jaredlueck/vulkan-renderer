#include "shadermodule.h"
#include <fstream>
#include <vector>
#include "utils.h"
#include "device.h"

ShaderModule::ShaderModule(Device* device, const char* shaderPath) {
    std::ifstream vertexShaderFile(shaderPath, std::ios::binary | std::ifstream::ate);
    size_t vertexShaderSize = vertexShaderFile.tellg();
    vertexShaderFile.seekg(0);
    std::vector<char> vertexShaderCode(vertexShaderSize);
    vertexShaderFile.read(vertexShaderCode.data(), vertexShaderSize);

    VkShaderModuleCreateInfo vertexShaderCreateInfo{};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderCode.data());
    vertexShaderCreateInfo.codeSize = vertexShaderCode.size();
    VkCall(vkCreateShaderModule(device->getHandle(), &vertexShaderCreateInfo, nullptr, &shaderModule));
}
