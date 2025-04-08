#pragma once

#include "VulkanContext.hpp"

namespace spoony::vkcore {

class WindowSurface;

class Swapchain final {
 public:
  Swapchain(ContextHandle context, const WindowSurface& surface);
  ~Swapchain();

 private:
  ContextHandle m_context;
  VkSwapchainKHR m_swapChain;
  std::vector<VkImage> m_images;
  std::vector<VkImageView> m_imageViews;
  std::vector<VkFramebuffer> m_framebuffers;
  VkFormat m_imageFormat;
  VkExtent2D m_extent;

  void createSwapChain(const WindowSurface& surface);
  void createImageViews();
  void createFrameBuffers();
};
}  // namespace spoony::vkcore