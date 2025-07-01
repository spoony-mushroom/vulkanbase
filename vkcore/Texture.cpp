#include "Texture.hpp"

#include "CommandBuffer.hpp"
#include "Utils.hpp"
#include "VulkanUtils.hpp"

using namespace spoony::utils;

namespace spoony::vkcore {

inline constexpr bool hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
}

inline constexpr uint32_t computeMipLevels(int width, int height) {
  // calculate number of times we can halve the image
  // add one for the base level 0
  return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) +
         1;
}

Texture::Texture(ContextHandle context, const TextureConfig& config)
    : m_context(context),
      m_width(config.width),
      m_height(config.height),
      m_format(config.format),
      m_mipLevels(config.mipLevels) {
  VkImageCreateInfo imageInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                              .imageType = VK_IMAGE_TYPE_2D,
                              .extent.width = m_width,
                              .extent.height = m_height,
                              .extent.depth = 1,
                              .mipLevels = m_mipLevels,
                              .arrayLayers = 1,
                              .format = m_format,
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

Texture::Texture(ContextHandle context,
                 uint32_t width,
                 uint32_t height,
                 std::span<uint8_t> data,
                 PixelFormat dataPixelFormat,
                 bool generateMipMaps,
                 VkFormat textureFormat)
    : Texture(
          context,
          {.width = width,
           .height = height,
           .mipLevels = generateMipMaps ? computeMipLevels(width, height) : 1,
           .format = textureFormat,
           .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT,
           .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT}) {
  VkDeviceSize imageSize = 0;
  switch (dataPixelFormat) {
    case RGBA:
      imageSize = width * height * 4;
      break;
    default:
      throw std::runtime_error("Invalid pixel data format");
  }

  assert(data.size() == imageSize);
  Buffer stagingBuffer(context, data, VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copyFrom(stagingBuffer);
  if (generateMipMaps) {
    generateMipmaps();
  }
}

Texture::~Texture() {
  vkDestroyImageView(m_context.device(), m_imageView, nullptr);
  vkDestroyImage(m_context.device(), m_image, nullptr);
  vkFreeMemory(m_context.device(), m_memory, nullptr);
}

void Texture::copyFrom(const Buffer& buffer) {
  AutoSubmitCommandBuffer cmdBuf(m_context);
  VkBufferImageCopy region{
      .bufferOffset = 0,
      .bufferRowLength = 0,    // indicates tightly packed
      .bufferImageHeight = 0,  // indicates tightly packed
      .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1},
      .imageOffset{.x = 0, .y = 0, .z = 0},
      .imageExtent{.width = m_width, .height = m_height, .depth = 1}};
  vkCmdCopyBufferToImage(
      cmdBuf, buffer, m_image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // current layout of the dest
                                             // image
      1, &region);
}

void Texture::transitionImageLayout(VkImageLayout oldLayout,
                                    VkImageLayout newLayout) {
  VkImageMemoryBarrier barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = m_image,
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = m_mipLevels,
                        .baseArrayLayer = 0,
                        .layerCount = 1}};

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (hasStencilComponent(m_format)) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }

  VkPipelineStageFlags srcStage, dstStage;
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;  // don't need to wait on anything
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    // since we're not waiting on anything, the source stage should be set to
    // the earliest possible stage in the pipeline
    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    // reads in early_fragment_tests, writes in late_fragment_tests
    // pick the earliest stage that needs the depth buffer
    dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    throw std::runtime_error("invalid image layout transition");
  }

  AutoSubmitCommandBuffer cmdBuf(m_context);
  // Common way to perform a layout transition is employing a memory barrier
  vkCmdPipelineBarrier(cmdBuf, srcStage, dstStage,
                       0,             // dependency flags
                       0, nullptr,    // memory barriers
                       0, nullptr,    // buffer memory barriers
                       1, &barrier);  // image memory barriers
}

void Texture::generateMipmaps() {
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(m_context.physicalDevice(), m_format,
                                      &formatProperties);
  if (!hasFlags(formatProperties.optimalTilingFeatures,
                VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error(
        "failed to generate mipmaps: linear filtering not supported");
  }

  AutoSubmitCommandBuffer cmdBuf(m_context);
  // need a barrier to transition each level individually
  // (our transitionImage function only does the entire image)
  VkImageMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                               .image = m_image,
                               .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .subresourceRange{
                                   .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                   .baseArrayLayer = 0,
                                   .layerCount = 1,
                                   .levelCount = 1,
                               }};

  int32_t mipWidth = m_width, mipHeight = m_height;

  for (uint32_t i = 1; i < m_mipLevels; i++) {
    // Make readable the base level (the one above the level we're creating)
    // barrier will wait on the base level to be filled, before we proceed
    // with blitting the next level
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    int32_t dstWidth = std::max(mipWidth / 2l, 1l);
    int32_t dstHeight = std::max(mipHeight / 2l, 1l);

    // for 2d images, the Z dimension has a size of 1
    VkImageBlit blit{.srcOffsets{{0, 0, 0}, {mipWidth, mipHeight, 1}},
                     .srcSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                     .baseArrayLayer = 0,
                                     .layerCount = 1,
                                     .mipLevel = i - 1},
                     .dstOffsets{{0, 0, 0}, {dstWidth, dstHeight, 1}},
                     .dstSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                     .baseArrayLayer = 0,
                                     .layerCount = 1,
                                     .mipLevel = i}};

    // We're blitting between different mip levels of the same VkImage,
    // doing a linear downscaling
    vkCmdBlitImage(cmdBuf, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    // Make the source mip image usable by the shader
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

    mipWidth = dstWidth;
    mipHeight = dstHeight;
  }

  // Barrier for the last mip level that wasn't handled in the loop
  barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
}
}  // namespace spoony::vkcore