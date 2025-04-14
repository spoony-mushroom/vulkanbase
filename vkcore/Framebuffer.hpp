#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"

namespace spoony::vkcore {
class RenderPass;
class Framebuffer {
 public:
  Framebuffer(ContextHandle context,
              std::span<VkImageView const> attachments,
              std::shared_ptr<RenderPass> renderPass,
              VkExtent2D extent);
  ~Framebuffer();

 private:
  ContextHandle m_context;
  VkFramebuffer m_framebuffer;
  std::shared_ptr<RenderPass> m_renderPass;
};
}  // namespace spoony::vkcore