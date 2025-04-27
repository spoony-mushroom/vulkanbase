#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"

namespace spoony::vkcore {

struct RenderPassConfig {
  VkFormat colorImageFormat;
  VkFormat depthStencilImageFormat;
  VkSampleCountFlagBits msaaSamples;
};
class RenderPass final {
 public:
  RenderPass(ContextHandle context, RenderPassConfig config);
  ~RenderPass();
  VkSampleCountFlagBits getSampleCount() const { return m_config.msaaSamples; }
  VkFormat getColorImageFormat() const { return m_config.colorImageFormat; }
  VkFormat getDepthImageFormat() const {
    return m_config.depthStencilImageFormat;
  }

  operator VkRenderPass() const { return m_renderPass; }

 private:
  ContextHandle m_context;
  VkRenderPass m_renderPass;
  RenderPassConfig m_config;
};
}  // namespace spoony::vkcore