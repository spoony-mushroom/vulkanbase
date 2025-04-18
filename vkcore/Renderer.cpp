#include "Renderer.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Pipeline.hpp"
#include "Utils.hpp"
#include "VulkanUtils.hpp"

namespace spoony::vkcore {
using namespace spoony::utils;

// TODO:
// RenderPass for unlit geometry
// Classes to hold Vertex and Texture data

static constexpr char VERT_SHADER_SPV[]{"shaders/triangle_app_vert.spv"};
static constexpr char FRAG_SHADER_SPV[]{"shaders/triangle_app_frag.spv"};

const std::vector<Vertex> vertices{
    {.pos{-0.5f, -0.5f, 0.f}, .color{1.f, 0, 0}, .texCoord{0, 1.f}},
    {.pos{0.5f, -0.5f, 0.f}, .color{0, 1.f, 0}, .texCoord{1.f, 1.f}},
    {.pos{0.5f, 0.5f, 0.f}, .color{0, 0, 1.f}, .texCoord{1.f, 0}},
    {.pos{-0.5f, 0.5f, 0.f}, .color{1.f, 1.f, 1.f}, .texCoord{0, 0}},

    {.pos{-0.5f, -0.5f, -0.5f}, .color{1.f, 0, 0}, .texCoord{0, 1.f}},
    {.pos{0.5f, -0.5f, -0.5f}, .color{0, 1.f, 0}, .texCoord{1.f, 1.f}},
    {.pos{0.5f, 0.5f, -0.5f}, .color{0, 0, 1.f}, .texCoord{1.f, 0}},
    {.pos{-0.5f, 0.5f, -0.5f}, .color{1.f, 1.f, 1.f}, .texCoord{0, 0}}};

// limited to 65535 vertices
const std::vector<uint16_t> indices{0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

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
  initFrameBuffers();
  initSyncObjects();
}

void Renderer::init() {
  m_context.initialize(*m_surface);
  m_swapChain = std::make_unique<Swapchain>(m_context, *m_surface);

  const RenderPassConfig renderPassConfig{
      .colorImageFormat = m_swapChain->getFormat(),
      .depthStencilImageFormat =
          utils::findDepthFormat(m_context.physicalDevice()),
      .msaaSamples = utils::getMaxUsableSampleCount(m_context.physicalDevice()),
  };

  m_renderPass = std::make_shared<RenderPass>(m_context, renderPassConfig);

  PipelineBuilder pipelineBuilder(m_context, m_renderPass);
  auto pipeline =
      pipelineBuilder.setMaxFramesInFlight(2)
          .setShaders(readFile(VERT_SHADER_SPV), readFile(FRAG_SHADER_SPV))
          .setVertexType<Vertex>()
          .addVertShaderUniform<UniformBufferObject>(0)
          .addTextureSampler(1)
          .create();

  m_pipeline = std::move(pipeline);
}

void Renderer::initFrameBuffers() {
  auto [width, height] = m_swapChain->getExtent();
  TextureConfig textureConfig{
      .width = width,
      .height = height,
      .format = m_swapChain->getFormat(),
      .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .numSamples = m_renderPass->getSampleCount(),
      .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT};
  m_colorRenderTexture = std::make_unique<Texture>(m_context, textureConfig);

  auto depthFormat = utils::findDepthFormat(m_context.physicalDevice());
  textureConfig.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  textureConfig.format = depthFormat;
  textureConfig.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
  m_depthRenderTexture = std::make_unique<Texture>(m_context, textureConfig);

  auto swapChainImageViews = m_swapChain->getImageViews();
  m_framebuffers.reserve(swapChainImageViews.size());
  for (size_t i = 0; i < m_framebuffers.size(); ++i) {
    std::array attachments{m_colorRenderTexture->getImageView(),
                           m_depthRenderTexture->getImageView(),
                           swapChainImageViews[i]};
    m_framebuffers.emplace_back(m_context, attachments, m_renderPass,
                                m_swapChain->getExtent());
  }
}

void Renderer::initSyncObjects() {
  for (int i = 0; i < k_maxFramesInFlight; i++) {
    m_imageAvailableSemaphores.emplace_back(m_context);
    m_renderFinishedSemaphores.emplace_back(m_context);
    m_inFlightFences.emplace_back(m_context);
  }
}

void Renderer::renderFrame() {}
}  // namespace spoony::vkcore