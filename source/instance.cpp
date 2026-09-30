#include "instance.h"
#include "utils.h"
#include <vector>
#include <iostream>
#include <format>
#include <cstring>
#include <ranges>
#include <GLFW/glfw3.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    (void)user_data;

    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        std::cout << std::format("{} Validation Layer: Error: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage) << std::endl;
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cout << std::format("{} Validation Layer: Warning: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage) << std::endl;
    }
    else if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        std::cout << std::format("{} Validation Layer: Performance warning: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage) << std::endl;
    }
    else
    {
        std::cout << std::format("{} Validation Layer: Information: {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage) << std::endl;
    }
    return VK_FALSE;
}

Instance::Instance() {
    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);

    uint32_t instance_extension_count;
    VkCall(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

    std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
    VkCall(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

    std::vector<const char*> required_instance_extensions{ VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

	for (int i = 0; i < count; ++i) {
		required_instance_extensions.push_back(extensions[i]);
	}

    std::vector<const char*> requested_instance_layers{ "VK_LAYER_KHRONOS_validation" };

    char const* validationLayer = "VK_LAYER_KHRONOS_validation";

    uint32_t instance_layer_count;
    VkCall(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

    std::vector<VkLayerProperties> supported_instance_layers(instance_layer_count);
    VkCall(vkEnumerateInstanceLayerProperties(&instance_layer_count, supported_instance_layers.data()));

    VkApplicationInfo app{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Vulkan Renderer",
    .pEngineName = "vulkan renderer",
    .apiVersion = VK_API_VERSION_1_1 };

    VkInstanceCreateInfo instance_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledLayerCount = static_cast<uint32_t>(requested_instance_layers.size()),
        .ppEnabledLayerNames = requested_instance_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(required_instance_extensions.size()),
        .ppEnabledExtensionNames = required_instance_extensions.data() };

    VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    
    debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_utils_create_info.pfnUserCallback = debug_callback;

    instance_info.pNext = &debug_utils_create_info;
 
    VkCall(vkCreateInstance(&instance_info, nullptr, &instance));
}

VkInstance Instance::getHandle() {
	return instance;
}
