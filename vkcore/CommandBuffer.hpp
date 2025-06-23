#pragma once

#include "VulkanContext.hpp"

namespace spoony::vkcore {
class CommandPool;

class CommandBuffer {
 public:
  CommandBuffer() = delete;
  CommandBuffer(VkCommandBuffer cmdBuf, std::shared_ptr<CommandPool> pool)
      : m_commandBuffer(cmdBuf), m_pool(pool) {}
  CommandBuffer(const CommandBuffer&) = delete;
  CommandBuffer(CommandBuffer&& other) noexcept;
  ~CommandBuffer();

  const VkCommandBuffer& get() const noexcept { return m_commandBuffer; }
  operator VkCommandBuffer() const noexcept { return get(); }
  CommandBuffer& operator=(const CommandBuffer&) = delete;

 private:
  VkCommandBuffer m_commandBuffer{VK_NULL_HANDLE};
  std::weak_ptr<CommandPool> m_pool;
};

class CommandPool final : public std::enable_shared_from_this<CommandPool> {
 public:
  CommandPool(ContextHandle context, uint32_t queue);
  ~CommandPool();
  CommandBuffer acquire(bool reset = true);
  void recycle(VkCommandBuffer cmdBuf);

 private:
  VkCommandPool m_pool;
  ContextHandle m_context;
};

}  // namespace spoony::vkcore