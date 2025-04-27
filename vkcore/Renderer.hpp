#pragma once
#include "CommandBuffer.hpp"
#include "Framebuffer.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "RenderPassModule.hpp"
#include "Swapchain.hpp"
#include "Synchronization.hpp"
#include "Texture.hpp"
#include "VulkanContext.hpp"
#include "WindowSurface.hpp"

namespace spoony::vkcore {

struct FrameContext {
  CommandBuffer cmdBuf;
  Semaphore imageAvailable;
  Semaphore renderFinished;
  Fence inFlight;

  FrameContext(ContextHandle context)
      : imageAvailable(context), renderFinished(context), inFlight(context) {}
};

class Renderer {
 public:
  template <typename TWind>
  Renderer(TWind* window);
  void drawFrame();
  void addRenderPassModule(std::unique_ptr<RenderPassModule> module);

 private:
  const int k_maxFramesInFlight{2};
  ContextHandle m_context;

  std::unique_ptr<WindowSurface> m_surface;
  std::unique_ptr<Swapchain> m_swapChain;
  // std::shared_ptr<RenderPass> m_renderPass;
  // std::unique_ptr<Pipeline> m_pipeline;

  // std::vector<Framebuffer> m_framebuffers;
  // std::unique_ptr<Texture> m_colorRenderTexture;
  // std::unique_ptr<Texture> m_depthRenderTexture;

  std::vector<FrameContext> m_frameContexts;

  std::map<std::thread::id, std::shared_ptr<CommandPool>> m_commandPools;

  uint32_t m_currentFrame;

  std::vector<std::unique_ptr<RenderPassModule>> m_renderModules;

  Renderer();
  void init();
  // void initFrameBuffers();
  void initFrameContexts();
  std::shared_ptr<CommandPool> getOrCreateCommandPool(std::thread::id id);
  CommandBuffer getCommandBuffer(bool reset = true);
};
}  // namespace spoony::vkcore