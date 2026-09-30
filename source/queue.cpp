#include "queue.h"
#include "utils.h"
#include "commandbuffer.h"

Queue::Queue(VkDevice device, uint32_t queueFamilyIndex, uint32_t index) {
	vkGetDeviceQueue(device, queueFamilyIndex, index, &queue);
}

void Queue::submit(CommandBuffer* commandBuffer, VkSemaphore* imageAvailableSemaphore, VkSemaphore* renderFinishedSemaphore, VkFence* inFlightFence) {
	// Submit work to the Vulkan queue here
	VkCommandBuffer vkbb = commandBuffer->getHandle();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkbb;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = renderFinishedSemaphore;

    VkCall(vkQueueSubmit(queue, 1, &submitInfo, *inFlightFence));
}

void Queue::present(VkSemaphore* renderFinishedSemaphore, VkSwapchainKHR swapchain, uint32_t imageIndex) {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = renderFinishedSemaphore;

    VkSwapchainKHR swapChains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkCall(vkQueuePresentKHR(queue, &presentInfo));
}