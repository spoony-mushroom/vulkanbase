#include "WindowSurface.hpp"

namespace spoony::vkcore {

WindowSurface::~WindowSurface() {
  vkDestroySurfaceKHR(m_context.instance(), m_surface, nullptr);
}

WindowSurface::WindowSurface(ContextHandle context) : m_context(context) {}

WindowSurfaceGLFW::WindowSurfaceGLFW(ContextHandle context, GLFWwindow* window)
    : WindowSurface(context) {
  glfwCreateWindowSurface(context.instance(), window, nullptr, &m_surface);
}

glm::ivec2 WindowSurfaceGLFW::getSize() const {
  glm::ivec2 size;
  glfwGetWindowSize(m_window, &size.x, &size.y);
  return size;
}
}  // namespace spoony::vkcore