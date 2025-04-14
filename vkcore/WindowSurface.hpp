#pragma once
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

template <typename T>
class WindowSurfaceImpl : public WindowSurface {
 public:
  WindowSurfaceImpl(ContextHandle context, T* window);
  glm::ivec2 getSize() const override;

 private:
  T* m_window;
};
}  // namespace spoony::vkcore