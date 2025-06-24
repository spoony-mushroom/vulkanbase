#pragma once

#include "VulkanContext.hpp"

namespace spoony::vkcore {

class CommandBuffer {
 public:
  CommandBuffer() = delete;
  CommandBuffer(const CommandBuffer&) = delete;
  CommandBuffer(CommandBuffer&& other) noexcept;
  ~CommandBuffer();

  static CommandBuffer acquire(ContextHandle context);

  const VkCommandBuffer& get() const noexcept { return m_commandBuffer; }
  operator VkCommandBuffer() const noexcept { return get(); }
  CommandBuffer& operator=(const CommandBuffer&) = delete;

 private:
  friend class CommandPool;
  CommandBuffer(VkCommandBuffer cmdBuf, std::shared_ptr<class CommandPool> pool)
      : m_commandBuffer(cmdBuf), m_pool(pool) {}
  VkCommandBuffer m_commandBuffer{VK_NULL_HANDLE};
  std::weak_ptr<CommandPool> m_pool;
};

class AutoSubmitCommandBuffer {
 public:
  AutoSubmitCommandBuffer(CommandBuffer cmdBuffer, VkQueue queue)
      : commandBuffer(std::move(cmdBuffer)), queue(queue) {
    VkCommandBufferBeginInfo beingInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    vkBeginCommandBuffer(commandBuffer, &beingInfo);
  }

  ~AutoSubmitCommandBuffer() {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                            .commandBufferCount = 1,
                            .pCommandBuffers = &commandBuffer.get()};

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
  }

  operator VkCommandBuffer() const noexcept { return commandBuffer; }

 private:
  CommandBuffer commandBuffer;
  VkQueue queue;
};

}  // namespace spoony::vkcore