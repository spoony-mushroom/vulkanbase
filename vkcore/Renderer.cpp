#include "Renderer.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Pipeline.hpp"
#include "Utils.hpp"
#include "VulkanUtils.hpp"

namespace spoony::vkcore {
using namespace spoony::utils;

// TODO:
// RenderPass for unlit geometry
// Classes to hold Vertex and Texture data

const std::vector<Vertex> vertices{
    {.pos{-0.5f, -0.5f, 0.f}, .color{1.f, 0, 0}, .texCoord{0, 1.f}},
    {.pos{0.5f, -0.5f, 0.f}, .color{0, 1.f, 0}, .texCoord{1.f, 1.f}},
    {.pos{0.5f, 0.5f, 0.f}, .color{0, 0, 1.f}, .texCoord{1.f, 0}},
    {.pos{-0.5f, 0.5f, 0.f}, .color{1.f, 1.f, 1.f}, .texCoord{0, 0}},

    {.pos{-0.5f, -0.5f, -0.5f}, .color{1.f, 0, 0}, .texCoord{0, 1.f}},
    {.pos{0.5f, -0.5f, -0.5f}, .color{0, 1.f, 0}, .texCoord{1.f, 1.f}},
    {.pos{0.5f, 0.5f, -0.5f}, .color{0, 0, 1.f}, .texCoord{1.f, 0}},
    {.pos{-0.5f, 0.5f, -0.5f}, .color{1.f, 1.f, 1.f}, .texCoord{0, 0}}};

// limited to 65535 vertices
const std::vector<uint16_t> indices{0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

static ContextHandle makeContext() {
  uint32_t glfwExtensionCount;
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  vkcore::VulkanContextBuilder contextBuilder;

  for (int i = 0; i < glfwExtensionCount; i++) {
    contextBuilder.addExtension(glfwExtensions[i]);
  }

  contextBuilder.setEnableValidation(true);
#ifdef NDEBUG
  contextBuilder.setEnableValidation(false);
#endif

  return contextBuilder.build();
}

Renderer::Renderer() : m_context(makeContext()) {};

template <>
Renderer::Renderer(GLFWwindow* window) : Renderer() {
  m_surface =
      std::make_unique<WindowSurfaceImpl<GLFWwindow>>(m_context, window);
  init();
  // initFrameBuffers();
  initFrameContexts();
}

void Renderer::init() {
  m_context.initialize(*m_surface);
  m_swapChain = std::make_unique<Swapchain>(m_context, *m_surface);
}

void Renderer::initFrameContexts() {
  for (int i = 0; i < k_maxFramesInFlight; i++) {
    m_frameContexts.emplace_back(m_context);
  }
}

void Renderer::drawFrame() {
  m_frameContexts[m_currentFrame].inFlight.wait(
      std::numeric_limits<uint64_t>::max());

  uint32_t imageIndex;
  VkResult result =
      vkAcquireNextImageKHR(m_context.device(), *m_swapChain, UINT64_MAX,
                            m_frameContexts[m_currentFrame].imageAvailable,
                            VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    // Not possible to present to the swap chain in this state
    m_swapChain = std::make_unique<Swapchain>(m_context, *m_surface);
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  m_frameContexts[m_currentFrame].inFlight.reset();

  auto cmdBuf = getCommandBuffer();

  for (auto& module : m_renderModules) {
    module->record(cmdBuf, imageIndex);
  }
  
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  // Wait for imageAvailableSemaphore at the
  // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage of the pipeline since
  // we want an image to be available before writing colors to it.
  VkSemaphore waitSemaphores[]{m_frameContexts[m_currentFrame].imageAvailable};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuf.get();

  VkSemaphore signalSemaphores[]{m_frameContexts[m_currentFrame].renderFinished};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VK_CHECK(vkQueueSubmit(m_context.get()->getGraphicsQueue(), 1, &submitInfo,
    m_frameContexts[m_currentFrame].inFlight), "submit draw command buffer");
  
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[]{*m_swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr;

  result = vkQueuePresentKHR(m_context.get()->getPresentQueue(), &presentInfo);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }

  m_currentFrame = (m_currentFrame + 1) % k_maxFramesInFlight;
}

std::shared_ptr<CommandPool> Renderer::getOrCreateCommandPool(
    std::thread::id id) {
  auto indices =
      utils::findQueueFamilies(m_context.physicalDevice(), *m_surface);

  auto itr = m_commandPools.find(id);
  if (itr == m_commandPools.end()) {
    auto commandPool = std::make_shared<CommandPool>(
        m_context, indices.graphicsFamily.value());
    m_commandPools[id] = commandPool;
    return commandPool;
  }
  return itr->second;
}

CommandBuffer Renderer::getCommandBuffer(bool reset) {
  auto pool = getOrCreateCommandPool(std::this_thread::get_id());
  return pool->acquire(reset);
}
}  // namespace spoony::vkcore