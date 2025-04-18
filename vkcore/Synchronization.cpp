#include "Synchronization.hpp"
#include <iostream>
#include "VulkanUtils.hpp"

namespace spoony::vkcore {
Fence::Fence(ContextHandle context) : m_context(context) {
  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  VK_CHECK(vkCreateFence(m_context.device(), &fenceInfo, nullptr, &m_fence),
           "create fence");
}

// Fence::Fence(Fence&& other) noexcept
//     : m_context(std::move(other.m_context)), m_fence(other.m_fence) {
//   other.m_fence = VK_NULL_HANDLE;
// }

Fence::~Fence() {
  if (m_context != nullptr)
    vkDestroyFence(m_context.device(), m_fence, nullptr);
}

Semaphore::Semaphore(ContextHandle context) : m_context(context) {
  VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VK_CHECK(vkCreateSemaphore(m_context.device(), &semaphoreInfo, nullptr,
                             &m_semaphore),
           "create semaphore");
}

// Semaphore::Semaphore(Semaphore&& other) noexcept
//     : m_context(std::move(other.m_context)), m_semaphore(other.m_semaphore) {
//   other.m_semaphore = VK_NULL_HANDLE;
// }

Semaphore::~Semaphore() {
  if (m_context != nullptr)
    vkDestroySemaphore(m_context.device(), m_semaphore, nullptr);
}
}  // namespace spoony::vkcore