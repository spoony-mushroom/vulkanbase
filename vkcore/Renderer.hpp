#pragma once
#include "CommandBuffer.hpp"
#include "Framebuffer.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Swapchain.hpp"
#include "Synchronization.hpp"
#include "Texture.hpp"
#include "VulkanContext.hpp"
#include "WindowSurface.hpp"

namespace spoony::vkcore {

class Renderer {
 public:
  template <typename TWind>
  Renderer(TWind* window);

 protected:
  Renderer();
  void init();
  void initFrameBuffers();
  void initSyncObjects();
  void renderFrame();

 private:
  const int k_maxFramesInFlight{2};
  ContextHandle m_context;
  std::unique_ptr<WindowSurface> m_surface;
  std::unique_ptr<Swapchain> m_swapChain;
  std::shared_ptr<RenderPass> m_renderPass;
  std::unique_ptr<Pipeline> m_pipeline;

  std::vector<Framebuffer> m_framebuffers;
  std::unique_ptr<Texture> m_colorRenderTexture;
  std::unique_ptr<Texture> m_depthRenderTexture;

  std::vector<CommandBuffer> m_commandBuffers;
  std::vector<Semaphore> m_imageAvailableSemaphores;
  std::vector<Semaphore> m_renderFinishedSemaphores;
  std::vector<Fence> m_inFlightFences;

  uint32_t m_currentFrame;
};
}  // namespace spoony::vkcore