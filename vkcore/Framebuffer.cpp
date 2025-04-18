#include "Framebuffer.hpp"

#include "RenderPass.hpp"

namespace spoony::vkcore {

Framebuffer::Framebuffer(ContextHandle context,
                         std::span<VkImageView const> attachments,
                         std::shared_ptr<RenderPass> renderPass,
                         VkExtent2D extent)
    : m_context(context), m_renderPass(renderPass) {
  VkFramebufferCreateInfo framebufferInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .renderPass = *m_renderPass,
      .width = extent.width,
      .height = extent.height,
      .layers = 1,  // single images
  };

  VK_CHECK(vkCreateFramebuffer(m_context.device(), &framebufferInfo, nullptr,
                               &m_framebuffer),
           "create framebuffer");
}

Framebuffer::~Framebuffer() {
  vkDestroyFramebuffer(m_context.device(), m_framebuffer, nullptr);
}
}  // namespace spoony::vkcore