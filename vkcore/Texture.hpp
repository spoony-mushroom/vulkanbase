#pragma once

#include "Buffer.hpp"
#include "VulkanContext.hpp"
namespace spoony::vkcore {

enum PixelFormat { RGBA };

struct TextureConfig {
  uint32_t width;
  uint32_t height;
  uint32_t mipLevels = 1;
  VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;
  VkFormat format;
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkImageUsageFlags usage;
  VkMemoryPropertyFlags memoryProperties;
  VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
};

class Texture {
 public:
  Texture(ContextHandle context, const TextureConfig& config);
  Texture(ContextHandle context,
          uint32_t width,
          uint32_t height,
          std::span<uint8_t> data,
          PixelFormat dataPixelFormat,
          bool generateMipMaps = true,
          VkFormat textureFormat = VK_FORMAT_R8G8B8A8_UNORM);
  ~Texture();

  void copyFrom(const Buffer& buffer);
  VkImageView getImageView() const { return m_imageView; }

 private:
  uint32_t m_width;
  uint32_t m_height;
  VkFormat m_format;
  uint32_t m_mipLevels;

  ContextHandle m_context;
  VkImage m_image;
  VkImageView m_imageView;
  VkDeviceMemory m_memory;

  void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
  void generateMipmaps();
};
}  // namespace spoony::vkcore