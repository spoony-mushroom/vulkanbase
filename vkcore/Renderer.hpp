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
  void registerCurrentThread();
  void drawFrame();

  template <typename T>
    requires std::derived_from<T, RenderPassModule>
  T& addRenderPassModule();

  VkExtent2D getExtent() const;
  void copyBuffer(const Buffer& src, Buffer& dst) const;
  ContextHandle getContext() const { return m_context; }

 private:
  const int k_maxFramesInFlight{2};
  ContextHandle m_context;

  std::unique_ptr<WindowSurface> m_surface;
  std::unique_ptr<Swapchain> m_swapChain;

  std::vector<FrameContext> m_frameContexts;

  std::mutex m_commandPoolMutex;
  std::map<std::thread::id, std::shared_ptr<CommandPool>> m_commandPools;

  uint32_t m_currentFrame;

  std::vector<std::unique_ptr<RenderPassModule>> m_renderModules;

  Renderer();
  void init();
  void initFrameContexts();
  std::shared_ptr<CommandPool> getCommandPool() const;
  CommandBuffer getCommandBuffer(bool reset = true) const;
};
}  // namespace spoony::vkcore