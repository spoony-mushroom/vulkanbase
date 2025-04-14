#include "Swapchain.hpp"

#include <array>
#include "VulkanUtils.hpp"
#include "WindowSurface.hpp"

namespace spoony::vkcore {

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    std::span<VkSurfaceFormatKHR const> formats) {
  // Prefer 32-bit SRGB, otherwise just return the first format available
  for (const auto& availableFormat : formats) {
    if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return formats.front();
}

static VkPresentModeKHR chooseSwapPresentMode(
    std::span<VkPresentModeKHR const> presentModes) {
  // Prefer "triple-buffering"
  for (const auto& availableMode : presentModes) {
    if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availableMode;
    }
  }
  // guaranteed to be available
  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                                   glm::ivec2 windowSize) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    // current extent is fine. Use it.
    return capabilities.currentExtent;
  } else {
    // Derive extent from window size
    VkExtent2D actualExtent{static_cast<uint32_t>(windowSize.x),
                            static_cast<uint32_t>(windowSize.y)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

Swapchain::Swapchain(ContextHandle context, const WindowSurface& surface)
    : m_context(context) {
  createSwapChain(surface);
  createImageViews();
}

Swapchain::~Swapchain() {
  vkDestroySwapchainKHR(m_context.device(), m_swapChain, nullptr);

  // no need to destroy m_images, as these are owned by the swapchain
  for (auto imageView : m_imageViews) {
    vkDestroyImageView(m_context.device(), imageView, nullptr);
  }
}

void Swapchain::createSwapChain(const WindowSurface& surface) {
  auto swapChainSupport =
      querySwapChainSupport(m_context.physicalDevice(), surface);
  auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  m_extent = chooseSwapExtent(swapChainSupport.capabilities, surface.getSize());

  // Add an extra image to the swap chain for triple-buffering
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = m_extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

  auto indices = findQueueFamilies(m_context.physicalDevice(), surface);
  uint32_t queueFamilyIndices[]{indices.graphicsFamily.value(),
                                indices.presentFamily.value()};
  if (indices.graphicsFamily != indices.presentFamily) {
    // Use concurrent mode to use multiple families in this swap chain. This
    // saves us from having to do ownership transfers, but is less performant.
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    // graphics and present are the same queue
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(m_context.device(), &createInfo, nullptr,
                           &m_swapChain) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create swap chain");
  }

  vkGetSwapchainImagesKHR(m_context.device(), m_swapChain, &imageCount,
                          nullptr);
  m_images.resize(imageCount);
  vkGetSwapchainImagesKHR(m_context.device(), m_swapChain, &imageCount,
                          m_images.data());
  m_imageFormat = surfaceFormat.format;
}

void Swapchain::createImageViews() {
  m_imageViews.resize(m_images.size());
  for (int i = 0; i < m_imageViews.size(); ++i) {
    m_imageViews[i] =
        createImageView(m_context.device(), m_images[i], m_imageFormat, 1,
                        VK_IMAGE_ASPECT_COLOR_BIT);
  }
}

}  // namespace spoony::vkcore