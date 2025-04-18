#include "Texture.hpp"
#include "VulkanUtils.hpp"

namespace spoony::vkcore {
Texture::Texture(ContextHandle context, const TextureConfig& config) : m_context(context) {
  VkImageCreateInfo imageInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                              .imageType = VK_IMAGE_TYPE_2D,
                              .extent.width = config.width,
                              .extent.height = config.height,
                              .extent.depth = 1,
                              .mipLevels = config.mipLevels,
                              .arrayLayers = 1,
                              .format = config.format,
                              .tiling = config.tiling,
                              .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                              .usage = config.usage,
                              .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                              .samples = config.numSamples};
  VK_CHECK(vkCreateImage(m_context.device(), &imageInfo, nullptr, &m_image),
           "create image");

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(m_context.device(), m_image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = utils::findMemoryType(m_context.physicalDevice(),
                                               memRequirements.memoryTypeBits,
                                               config.memoryProperties)};

  VK_CHECK(vkAllocateMemory(m_context.device(), &allocInfo, nullptr, &m_memory),
           "allocate image memory");

  vkBindImageMemory(m_context.device(), m_image, m_memory, 0);

  VkImageViewCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = m_image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = config.format,
      .subresourceRange{.aspectMask = config.aspectFlags,
                        .baseMipLevel = 0,
                        .levelCount = config.mipLevels,
                        .baseArrayLayer = 0,
                        .layerCount = 1}};

  VK_CHECK(
      vkCreateImageView(m_context.device(), &createInfo, nullptr, &m_imageView),
      "create image view");
}

Texture::~Texture() {
    vkDestroyImageView(m_context.device(), m_imageView, nullptr);
    vkDestroyImage(m_context.device(), m_image, nullptr);
    vkFreeMemory(m_context.device(), m_memory, nullptr);
}
}  // namespace spoony::vkcore