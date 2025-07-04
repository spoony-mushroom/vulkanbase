#pragma once
#include <set>

#include "Framebuffer.hpp"
#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "RenderPassModule.hpp"
#include "Swapchain.hpp"
#include "Texture.hpp"

namespace spoony::vkcore {
class UnlitRenderPass : public RenderPassModule {
 public:
  UnlitRenderPass(ContextHandle context,
                  const RenderPassConfig& renderPassConfig,
                  int maxFramesInFlight);
  void selectOutput(int framebufferIndex) override;
  void record(VkCommandBuffer cmdBuf, uint32_t imageIndex) override;
  void setOutputAttachments(std::span<const VkImageView> outputAttachments,
                            VkExtent2D extent) override;

  void setModelViewProjection(glm::mat4 model, glm::mat4 view, glm::mat4 proj);
  void addObject(Mesh&& mesh, Texture&& texture);

 private:
  const int k_maxFramesInFlight;
  ContextHandle m_context;
  std::unique_ptr<Pipeline> m_pipeline;
  RenderPass m_renderPass;
  std::vector<Framebuffer> m_framebuffers;
  std::unique_ptr<Texture> m_colorRenderTexture;
  std::unique_ptr<Texture> m_depthRenderTexture;
  std::vector<std::tuple<Mesh, Texture>> m_objects;
  // std::set<std::shared_ptr<Mesh>> m_meshes;
  // std::vector<std::shared_ptr<Texture>> m_textures;

  Framebuffer* m_activeFramebuffer;

  void initPipeline();
  void initFramebuffers(std::span<const VkImageView> outputAttachments,
                        VkExtent2D extent);
  void createDescriptorSetLayout();
};
}  // namespace spoony::vkcore