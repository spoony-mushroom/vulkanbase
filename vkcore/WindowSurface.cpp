#include "WindowSurface.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace spoony::vkcore {

WindowSurface::~WindowSurface() {
  vkDestroySurfaceKHR(m_context.instance(), m_surface, nullptr);
}

WindowSurface::WindowSurface(ContextHandle context) : m_context(context) {}

template <>
WindowSurfaceImpl<GLFWwindow>::WindowSurfaceImpl(ContextHandle context,
                                                 GLFWwindow* window)
    : WindowSurface(context), m_window(window) {
  glfwCreateWindowSurface(context.instance(), m_window, nullptr, &m_surface);
}

template <>
glm::ivec2 WindowSurfaceImpl<GLFWwindow>::getSize() const {
  glm::ivec2 size;
  glfwGetWindowSize(m_window, &size.x, &size.y);
  return size;
}
}  // namespace spoony::vkcore