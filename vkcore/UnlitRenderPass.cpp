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

void UnlitRenderPass::selectOutput(int framebufferIndex) {
  m_activeFramebuffer = &m_framebuffers[framebufferIndex];
}

void UnlitRenderPass::record(VkCommandBuffer cmdBuf, uint32_t imageIndex) {
  assert(m_activeFramebuffer != nullptr);
  auto renderScope = m_renderPass.createScope(cmdBuf, *m_activeFramebuffer,
                                              m_activeFramebuffer->getExtent());
  m_pipeline->bind(cmdBuf);
  m_pipeline->bindUniforms(cmdBuf, imageIndex);
  for (auto mesh : m_meshes) {
    mesh->draw(cmdBuf);
  }
}

void UnlitRenderPass::setOutputAttachments(
    std::span<const VkImageView> outputAttachments,
    VkExtent2D extent) {
  initFramebuffers(outputAttachments, extent);
}

void UnlitRenderPass::setModelViewProjection(glm::mat4 model,
                                             glm::mat4 view,
                                             glm::mat4 proj) {
  UniformBufferObject ubo{model, view, proj};
  m_pipeline->updateUniform(0, ubo);
}

void UnlitRenderPass::addMesh(std::shared_ptr<Mesh> mesh) {
  m_meshes.insert(mesh);
}

void UnlitRenderPass::initPipeline() {
  PipelineBuilder pipelineBuilder(m_context, m_renderPass);
  m_pipeline =
      pipelineBuilder.setMaxFramesInFlight(k_maxFramesInFlight)
          .setShaders(readFile(VERT_SHADER_SPV), readFile(FRAG_SHADER_SPV))
          .setVertexType<Vertex>()
          .addVertShaderUniform<UniformBufferObject>(0)
          // .addTextureSampler(1)
          .create();
}

void UnlitRenderPass::initFramebuffers(
    std::span<const VkImageView> outputAttachments,
    VkExtent2D extent) {
  TextureConfig textureConfig{
      .width = extent.width,
      .height = extent.height,
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

  m_framebuffers.clear();
  m_framebuffers.reserve(outputAttachments.size());
  for (const auto& outputAttachment : outputAttachments) {
    std::array attachments{m_colorRenderTexture->getImageView(),
                           m_depthRenderTexture->getImageView(),
                           outputAttachment};
    m_framebuffers.emplace_back(m_context, attachments, m_renderPass, extent);
  }
}
}  // namespace spoony::vkcore