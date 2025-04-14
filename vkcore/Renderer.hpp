#pragma once
#include "RenderPass.hpp"
#include "Swapchain.hpp"
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

 private:
  ContextHandle m_context;
  std::unique_ptr<WindowSurface> m_surface;
  std::unique_ptr<Swapchain> m_swapChain;
  std::shared_ptr<RenderPass> m_renderPass;
};
}  // namespace spoony::vkcore