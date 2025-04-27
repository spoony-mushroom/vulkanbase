#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"

namespace spoony::vkcore {
class Fence final {
 public:
  Fence(ContextHandle context);
  Fence(const Fence&) = delete;
  Fence(Fence&&) noexcept = default;

  ~Fence();
  operator VkFence() const { return m_fence; }
  const VkFence& get() const { return m_fence; }
  void reset() const;
  void wait(uint64_t timeout) const;

 private:
  ContextHandle m_context;
  VkFence m_fence;
};
class Semaphore final {
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