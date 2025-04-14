#include "Renderer.hpp"
#include "Pipeline.hpp"
#include "VulkanUtils.hpp"

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

Renderer::Renderer() : m_context(makeContext()) {};

template <>
Renderer::Renderer(GLFWwindow* window) : Renderer() {
  m_surface =
      std::make_unique<WindowSurfaceImpl<GLFWwindow>>(m_context, window);
  init();
}

void Renderer::init() {
  m_context.initialize(*m_surface);
  m_swapChain = std::make_unique<Swapchain>(m_context, *m_surface);

  const RenderPassConfig renderPassConfig{
      .colorImageFormat = m_swapChain->getFormat(),
      .depthStencilImageFormat = findDepthFormat(m_context.physicalDevice()),
      .msaaSamples = getMaxUsableSampleCount(m_context.physicalDevice()),
  };

  m_renderPass = std::make_shared<RenderPass>(m_context, renderPassConfig);

  PipelineBuilder pipelineBuilder(m_context, m_renderPass);
  pipelineBuilder.setMaxFramesInFlight(2)
      .setVertexType<Vertex>()
      .addVertShaderUniform<UniformBufferObject>(0)
      .addTextureSampler(1);
}
}  // namespace spoony::vkcore