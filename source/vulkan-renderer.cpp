#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>
#include "swapchain.h"
#include "utils.h"
#include "shadermodule.h"
#include "instance.h"
#include "renderpass.h"
#include "renderpipeline.h"
#include "device.h"
#include "physicaldevice.h"
#include "commandpool.h"
#include "commandbuffer.h"
#include "framebuffer.h"
#include "queue.h"
#include "cube.h"
#include "math.h"
#include "framedata.h"
#include "tinygltf/tiny_gltf_v3.h"
#include "model.h"
#include <chrono>

void window_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

bool recreateSwapchain = false;
bool dragging = false;
double xStart, yStart;

glm::mat4 modelMatrix(1.0);

int main()
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "vulkan renderer", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // Create vulkan instance
	Instance instance;

    VkSurfaceKHR surface;
    VkCall(glfwCreateWindowSurface(instance.getHandle(), window, NULL, &surface));

	PhysicalDevice physicalDevice(instance.getHandle(), surface);

	// create logical device to interface with the physical device
	Device device(&physicalDevice);
    
    Swapchain* swapchain = new Swapchain(&device, surface, 3);

	VkSurfaceFormatKHR surfaceFormat = swapchain->getSurfaceFormat();

	VkSurfaceCapabilitiesKHR capabilities = swapchain->getCapabilities();
    
	ShaderModule vertexShaderModule(&device, "./shaders/vert.spv");
    ShaderModule fragmentShaderModule(&device, "./shaders/frag.spv");

	RenderPass renderPass(&device, surfaceFormat.format, VK_FORMAT_D32_SFLOAT);
    swapchain->createFramebuffers(&renderPass);

	RenderPipeline pipeline(&vertexShaderModule, &fragmentShaderModule, &renderPass, &device);
	renderPass.setPipeline(&pipeline);

    VkPipelineLayout layout = pipeline.getLayout();

	swapchain->createFramebuffers(&renderPass);

	CommandPool commandPool(&device, physicalDevice.getGraphicsQueueFamilyIndex());
	CommandBuffer commandBuffer = *commandPool.allocateCommandBuffer();

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	std::vector<VkSemaphore> acquireSemaphores(3);
	std::vector<VkSemaphore> releaseSemaphores(3);

	std::vector<VkSemaphore> recycledSemaphores;

    tg3_parse_options opts;
    tg3_error_stack errors;
    tg3_model model;

    tg3_parse_options_init(&opts);
    tg3_error_stack_init(&errors);

    const char* filename2 = "./assets/dragon2/dragon.gltf";
    
    Model dragon(&device, filename2, &pipeline);

    VkImageCreateInfo depthAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    };

    VkCall(vkCreateFence(device.getHandle(), &fenceInfo, nullptr, &inFlightFence));

    auto lastTime = std::chrono::high_resolution_clock::now();

    uint32_t currentBuffer = 0;
  
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        // wait for previous frame
        VkCall(vkWaitForFences(device.getHandle(), 1, &inFlightFence, VK_TRUE, UINT64_MAX));

        VkCall(vkResetFences(device.getHandle(), 1, &inFlightFence));

        VkSemaphore acquireSemaphore;
        if (recycledSemaphores.empty()) 
        {
            acquireSemaphore = device.createSemaphore(&semaphoreInfo);
        }
        else 
        {
			acquireSemaphore = recycledSemaphores.back();
			recycledSemaphores.pop_back();
        }

        if (recreateSwapchain) {
            Swapchain* newSwapchain = new Swapchain(&device, swapchain, surface, 3);
            delete swapchain;
            swapchain = newSwapchain;
            swapchain->createFramebuffers(&renderPass);
            recreateSwapchain = false;
        }
        
        // acquire image from the swap chain
        uint32_t imageIndex;
		swapchain->acquireNextImage(&imageIndex, acquireSemaphore, inFlightFence);

		VkSemaphore oldSemaphore = acquireSemaphores[imageIndex];
        if (oldSemaphore) {
			recycledSemaphores.push_back(oldSemaphore);
        }

		acquireSemaphores[imageIndex] = acquireSemaphore;

        VkExtent2D extent = swapchain->getExtent();

        if (extent.height == 0 || extent.width == 0) {
            continue;
        }

        commandBuffer.reset();

		commandBuffer.begin();

        Framebuffer* framebuffer = swapchain->getFramebuffer(imageIndex);

        float aspectRatio = (float)extent.width / (float)extent.height;

        glm::vec3 cameraPos(0.0, 0.0, -50.0);

        FrameData framedata{
            .model = modelMatrix,
            .view = matrix::lookAt(cameraPos, glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0)),
            .projection = matrix::perspective(0.1, 100.0, 65, aspectRatio),
            .lightDir = glm::vec4(-3.0, -5.0, 0.0, 0.0),
            .cameraPos = glm::vec4(cameraPos, 1.0)
        };

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewport.height = extent.height;
        viewport.width = extent.width;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        RenderPass renderPass(&device, surfaceFormat.format, VK_FORMAT_D32_SFLOAT);

        renderPass.begin(&commandBuffer, framebuffer, extent, viewport, scissor);

        pipeline.setUniformData(&framedata, sizeof(framedata));

        pipeline.bind(&commandBuffer);

        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        
        lastTime = currentTime;

        dragon.setPipeline(&pipeline);
        //dragon.updateAnimations(deltaTime, currentBuffer);

        dragon.draw(&commandBuffer);

        currentBuffer = (currentBuffer + 1) % 3;

        renderPass.end(&commandBuffer);
        
        commandBuffer.end();

        Queue* graphicsQueue = device.getGraphicsQueue(0);

        if (!releaseSemaphores[imageIndex]) {
			releaseSemaphores[imageIndex] = device.createSemaphore(&semaphoreInfo);
        }

        graphicsQueue->submit(&commandBuffer, &acquireSemaphores[imageIndex], &releaseSemaphores[imageIndex], &inFlightFence);

        graphicsQueue->present(&releaseSemaphores[imageIndex], swapchain->getHandle(), imageIndex);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    recreateSwapchain = true;
    return;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        dragging = true;
        glfwGetCursorPos(window, &xStart, &yStart);
        return;
    }
        
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        dragging = false;
        return;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (dragging) {
        double deltaX = xpos - xStart;
        double deltaY = ypos - yStart;

        glm::mat4 tx = matrix::rotate(glm::radians(deltaY/100.0), glm::vec3(1.0, 0.0, 0.0));
        glm::mat4 ty = matrix::rotate(glm::radians(deltaX/100.0), glm::vec3(0.0, 1.0, 0.0));

        modelMatrix = ty * tx * modelMatrix;
    }
}