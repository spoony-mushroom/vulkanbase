#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"

namespace spoony::vkcore {
class RenderPass;
class Framebuffer {
 public:
  Framebuffer(ContextHandle context,
              std::span<VkImageView const> attachments,
              VkRenderPass renderPass,
              VkExtent2D extent);
  ~Framebuffer();

  VkExtent2D getExtent() const { return m_extent; }
  operator VkFramebuffer() const { return m_framebuffer; }

 private:
  VkExtent2D m_extent{};
  ContextHandle m_context;
  VkFramebuffer m_framebuffer;
};
}  // namespace spoony::vkcore