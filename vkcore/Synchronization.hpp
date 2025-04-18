#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"

namespace spoony::vkcore {
class Fence {
 public:
  Fence(ContextHandle context);
  Fence(const Fence&) = delete;
  Fence(Fence&&) noexcept = default;

  ~Fence();
  operator VkFence() { return m_fence; }

 private:
  ContextHandle m_context;
  VkFence m_fence;
};
class Semaphore {
 public:
  Semaphore(ContextHandle context);
  Semaphore(const Semaphore&) = delete;
  Semaphore(Semaphore&&) noexcept = default;

  ~Semaphore();
  operator VkSemaphore() { return m_semaphore; }

 private:
  ContextHandle m_context;
  VkSemaphore m_semaphore;
};
}  // namespace spoony::vkcore