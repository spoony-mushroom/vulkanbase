#include "UnlitRenderPass.hpp"

#include "Texture.hpp"
#include "Utils.hpp"

namespace spoony::vkcore {
using namespace spoony::utils;

static constexpr char VERT_SHADER_SPV[]{"shaders/triangle_app_vert.spv"};
static constexpr char FRAG_SHADER_SPV[]{"shaders/triangle_app_frag.spv"};

UnlitRenderPass::UnlitRenderPass(ContextHandle context,
                                 const RenderPassConfig& renderPassConfig,
                                 int maxFramesInFlight)
    : m_context(context),
      k_maxFramesInFlight(maxFramesInFlight),
      m_renderPass(context, renderPassConfig) {
  initPipeline();
}

void UnlitRenderPass::record(VkCommandBuffer cmdBuf, uint32_t imageIndex) {}

void UnlitRenderPass::setOutputAttachments(
    std::span<const VkImageView> outputAttachments,
    VkExtent2D extent) {
  initFrameBuffers(outputAttachments, extent);
}

void UnlitRenderPass::initPipeline() {
  PipelineBuilder pipelineBuilder(m_context, m_renderPass);
  m_pipeline =
      pipelineBuilder.setMaxFramesInFlight(k_maxFramesInFlight)
          .setShaders(readFile(VERT_SHADER_SPV), readFile(FRAG_SHADER_SPV))
          .setVertexType<Vertex>()
          .addVertShaderUniform<UniformBufferObject>(0)
          .addTextureSampler(1)
          .create();
}
void UnlitRenderPass::initFrameBuffers(
    std::span<const VkImageView> outputAttachments,
    VkExtent2D extent) {
  TextureConfig textureConfig{
      .width = extent.width,
      .height = extent.width,
      .format = m_renderPass.getColorImageFormat(),
      .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .numSamples = m_renderPass.getSampleCount(),
      .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT};

  m_colorRenderTexture = std::make_unique<Texture>(m_context, textureConfig);

  auto depthFormat = utils::findDepthFormat(m_context.physicalDevice());
  textureConfig.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  textureConfig.format = depthFormat;
  textureConfig.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
  m_depthRenderTexture = std::make_unique<Texture>(m_context, textureConfig);

  m_framebuffers.reserve(outputAttachments.size());
  for (size_t i = 0; i < m_framebuffers.size(); ++i) {
    std::array attachments{m_colorRenderTexture->getImageView(),
                           m_depthRenderTexture->getImageView(),
                           outputAttachments[i]};
    m_framebuffers.emplace_back(m_context, attachments, m_renderPass, extent);
  }
}
}  // namespace spoony::vkcore