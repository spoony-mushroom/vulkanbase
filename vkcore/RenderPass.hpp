#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"

namespace spoony::vkcore {

class Framebuffer;

struct RenderPassConfig {
  VkFormat colorImageFormat;
  VkFormat depthStencilImageFormat;
  VkSampleCountFlagBits msaaSamples;
};
class RenderPass final {
 public:
  RenderPass(ContextHandle context, const RenderPassConfig& config);
  ~RenderPass();
  VkSampleCountFlagBits getSampleCount() const { return m_sampleCount; }

  operator VkRenderPass() const { return m_renderPass; }

 private:
  ContextHandle m_context;
  VkRenderPass m_renderPass;
  VkSampleCountFlagBits m_sampleCount;
};
}  // namespace spoony::vkcore