#pragma once
#include "VulkanContext.hpp"
#include "Swapchain.hpp"

namespace spoony::vkcore {

class Renderer {
 public:
  Renderer(ContextHandle context, VkSurfaceKHR surface);

 private:
  ContextHandle m_context;
};
}  // namespace spoony::vkcore