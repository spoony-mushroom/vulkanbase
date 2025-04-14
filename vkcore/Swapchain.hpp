#pragma once

#include "VulkanContext.hpp"

namespace spoony::vkcore {

class WindowSurface;

class Swapchain final {
 public:
  Swapchain(ContextHandle context, const WindowSurface& surface);
  ~Swapchain();

  VkFormat getFormat() const { return m_imageFormat; }
  VkExtent2D getExtent() const { return m_extent; }
  std::span<VkImageView const> getImageViews() const { return m_imageViews; }

 private:
  ContextHandle m_context;
  VkSwapchainKHR m_swapChain;
  std::vector<VkImage> m_images;
  std::vector<VkImageView> m_imageViews;
  VkFormat m_imageFormat;
  VkExtent2D m_extent;

  void createSwapChain(const WindowSurface& surface);
  void createImageViews();
};
}  // namespace spoony::vkcore