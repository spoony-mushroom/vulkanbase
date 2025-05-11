#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"

namespace spoony::vkcore {

struct RenderPassConfig {
  VkFormat colorImageFormat;
  VkFormat depthStencilImageFormat;
  VkSampleCountFlagBits msaaSamples;
};

struct RenderPassScope final {
 public:
  ~RenderPassScope();

 private:
  RenderPassScope(VkCommandBuffer cmdBuf);
  VkCommandBuffer m_cmdBuf;

  friend class RenderPass;
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

  RenderPassScope createScope(VkCommandBuffer cmdBuf,
                              VkFramebuffer framebuffer,
                              VkExtent2D extent) const;

  operator VkRenderPass() const { return m_renderPass; }

 private:
  ContextHandle m_context;
  VkRenderPass m_renderPass;
  RenderPassConfig m_config;
};
}  // namespace spoony::vkcore