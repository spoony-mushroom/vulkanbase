#pragma once
#include "Swapchain.hpp"
#include "VulkanContext.hpp"
#include "WindowSurface.hpp"

namespace spoony::vkcore {

class Renderer {
 public:
  Renderer(GLFWwindow* window);

 private:
  ContextHandle m_context;
  WindowSurfaceGLFW m_surface;
};
}  // namespace spoony::vkcore