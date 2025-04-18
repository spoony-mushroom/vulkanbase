#pragma once

#include "VulkanContext.hpp"
namespace spoony::vkcore {

struct TextureConfig {
  uint32_t width;
  uint32_t height;
  uint32_t mipLevels = 1;
  VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;
  VkFormat format;
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkImageUsageFlags usage;
  VkMemoryPropertyFlags memoryProperties;
  VkImageAspectFlags aspectFlags;
};

class Texture {
 public:
  Texture(ContextHandle context, const TextureConfig& config);
  ~Texture();

  VkImageView getImageView() const { return m_imageView; }

 private:
  uint32_t m_width;
  uint32_t m_height;
  VkFormat m_format;
  ContextHandle m_context;
  VkImage m_image;
  VkImageView m_imageView;
  VkDeviceMemory m_memory;
};
}  // namespace spoony::vkcore