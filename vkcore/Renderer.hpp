#pragma once
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
  ~Renderer();
  void drawFrame();
  void refreshSwapChain();

  template <typename T>
    requires std::derived_from<T, RenderPassModule>
  T& addRenderPassModule();

  VkExtent2D getExtent() const;
  ContextHandle getContext() const { return m_context; }

 private:
  const int k_maxFramesInFlight{2};
  ContextHandle m_context;

  std::unique_ptr<WindowSurface> m_surface;
  std::unique_ptr<Swapchain> m_swapChain;

  std::vector<FrameContext> m_frameContexts;

  uint32_t m_currentFrame{0};

  std::vector<std::unique_ptr<RenderPassModule>> m_renderModules;

  Renderer();
  void init();
  void initFrameContexts();
};
}  // namespace spoony::vkcore