#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanContext.hpp"

namespace spoony::vkcore {
class WindowSurface final {
 public:
  WindowSurface(ContextHandle context, GLFWwindow* window)
      : m_context(context) {}
  ~WindowSurface() {
    vkDestroySurfaceKHR(m_context.instance(), m_surface, nullptr);
  }

 private:
  ContextHandle m_context;
  VkSurfaceKHR m_surface;
};

class Swapchain final {};
}  // namespace spoony::vkcore