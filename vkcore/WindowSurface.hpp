#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "VulkanContext.hpp"

namespace spoony::vkcore {
class WindowSurface {
 public:
  virtual ~WindowSurface();
  virtual glm::ivec2 getSize() const = 0;

  VkSurfaceKHR getSurface() const { return m_surface; }

  operator VkSurfaceKHR() const { return getSurface(); }

 protected:
  WindowSurface(ContextHandle context);
  VkSurfaceKHR m_surface;

 private:
  ContextHandle m_context;
};

// TODO: move GLFW-specific stuff to another folder
class WindowSurfaceGLFW : public WindowSurface {
 public:
  WindowSurfaceGLFW(ContextHandle context, GLFWwindow* window);

  glm::ivec2 getSize() const override;

 private:
  GLFWwindow* m_window;
};
}  // namespace spoony::vkcore