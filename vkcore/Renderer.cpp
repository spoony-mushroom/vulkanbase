#include "Renderer.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace spoony::vkcore {
static ContextHandle makeContext() {
  uint32_t glfwExtensionCount;
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  vkcore::VulkanContextBuilder contextBuilder;

  for (int i = 0; i < glfwExtensionCount; i++) {
    contextBuilder.addExtension(glfwExtensions[i]);
  }

  contextBuilder.setEnableValidation(true);
#ifdef NDEBUG
  contextBuilder.setEnableValidation(false);
#endif

  return contextBuilder.build();
}

Renderer::Renderer(GLFWwindow* window)
    : m_context(makeContext()), m_surface(m_context, window) {
  m_context.initialize(m_surface);
}
}  // namespace spoony::vkcore