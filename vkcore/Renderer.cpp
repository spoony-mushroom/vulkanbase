#include "Renderer.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Pipeline.hpp"
#include "Utils.hpp"
#include "VulkanUtils.hpp"

// Render pass implementations
#include "UnlitRenderPass.hpp"

namespace spoony::vkcore {
using namespace spoony::utils;

class AutoSubmitCommandBuffer {
 public:
  AutoSubmitCommandBuffer(CommandBuffer cmdBuffer, VkQueue queue)
      : commandBuffer(std::move(cmdBuffer)), queue(queue) {
    VkCommandBufferBeginInfo beingInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    vkBeginCommandBuffer(commandBuffer, &beingInfo);
  }

  ~AutoSubmitCommandBuffer() {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                            .commandBufferCount = 1,
                            .pCommandBuffers = &commandBuffer.get()};

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
  }

  operator VkCommandBuffer() const noexcept { return commandBuffer; }

 private:
  CommandBuffer commandBuffer;
  VkQueue queue;
};

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

Renderer::~Renderer() {
  m_context.get()->deviceWaitIdle();
}

void Renderer::registerCurrentThread() {
  auto threadId = std::this_thread::get_id();
  auto indices =
      utils::findQueueFamilies(m_context.physicalDevice(), *m_surface);

  std::unique_lock lck(m_commandPoolMutex);
  if (auto itr = m_commandPools.find(threadId); itr == m_commandPools.end()) {
    m_commandPools[threadId] = std::make_shared<CommandPool>(
        m_context, indices.graphicsFamily.value());
  }
}

void Renderer::drawFrame() {
  // assert(m_frameContexts.size() > m_currentFrame);
  // assert(m_frameContexts[m_currentFrame].inFlight != nullptr);
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

  static constexpr VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  auto cmdBuf = getCommandBuffer();

  VK_CHECK(vkBeginCommandBuffer(cmdBuf, &beginInfo),
           "begin recording command buffer");

  for (auto& module : m_renderModules) {
    module->selectOutput(imageIndex);
    module->record(cmdBuf, m_currentFrame);
  }

  VK_CHECK(vkEndCommandBuffer(cmdBuf), "record command buffer");

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

  VkSemaphore signalSemaphores[]{
      m_frameContexts[m_currentFrame].renderFinished};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VK_CHECK(vkQueueSubmit(m_context.get()->getGraphicsQueue(), 1, &submitInfo,
                         m_frameContexts[m_currentFrame].inFlight),
           "submit draw command buffer");

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

void Renderer::refreshSwapChain() {
  m_context.get()->deviceWaitIdle();
  m_swapChain.reset();
  m_swapChain = std::make_unique<Swapchain>(m_context, *m_surface);
  for (auto& renderModule : m_renderModules) {
    renderModule->setOutputAttachments(m_swapChain->getImageViews(),
                                       m_swapChain->getExtent());
  }
}

VkExtent2D Renderer::getExtent() const {
  return m_swapChain->getExtent();
}

void Renderer::copyBuffer(const Buffer& src, Buffer& dst) const {
  auto scope = AutoSubmitCommandBuffer(getCommandBuffer(),
                                       m_context.get()->getGraphicsQueue());
  VkBufferCopy copyRegion{.size = src.getSize()};
  vkCmdCopyBuffer(scope, src, dst, 1, &copyRegion);
}

std::shared_ptr<CommandPool> Renderer::getCommandPool() const {
  auto id = std::this_thread::get_id();
  auto itr = m_commandPools.find(id);
  if (itr == m_commandPools.end()) {
    throw std::runtime_error(
        "Unable to get a command pool. Did you call registerCurrentThread()?");
  }
  return itr->second;
}

CommandBuffer Renderer::getCommandBuffer(bool reset) const {
  auto pool = getCommandPool();
  return pool->acquire(reset);
}

template <>
UnlitRenderPass& Renderer::addRenderPassModule<UnlitRenderPass>() {
  RenderPassConfig config{
      .colorImageFormat = m_swapChain->getFormat(),
      .depthStencilImageFormat =
          utils::findDepthFormat(m_context.physicalDevice()),
      .msaaSamples =
          utils::getMaxUsableSampleCount(m_context.physicalDevice())};
  m_renderModules.push_back(
      std::make_unique<UnlitRenderPass>(m_context, config, 2));

  UnlitRenderPass* renderPass =
      static_cast<UnlitRenderPass*>(m_renderModules.back().get());

  renderPass->setOutputAttachments(m_swapChain->getImageViews(),
                                   m_swapChain->getExtent());
  return *renderPass;
}
}  // namespace spoony::vkcore