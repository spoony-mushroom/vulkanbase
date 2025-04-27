#include "CommandBuffer.hpp"

namespace spoony::vkcore {
CommandPool::CommandPool(ContextHandle context, uint32_t queue)
    : m_context(context) {
  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queue};

  VkCommandPool commandPool;
  VK_CHECK(vkCreateCommandPool(context.device(), &poolInfo, nullptr, &m_pool),
           "create command pool");
}

CommandPool::~CommandPool() {
  vkDestroyCommandPool(m_context.device(), m_pool, nullptr);
}

CommandBuffer CommandPool::acquire(bool reset) {
  VkCommandBuffer cmdBuf{VK_NULL_HANDLE};
  if (m_availableBuffers.empty()) {
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_pool,
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY};

    VK_CHECK(vkAllocateCommandBuffers(m_context.device(), &allocInfo, &cmdBuf),
             "allocate command buffer");
  } else {
    cmdBuf = m_availableBuffers.back();
    m_availableBuffers.pop_back();
    if (reset) {
        vkResetCommandBuffer(cmdBuf, 0);
    }
  }
  return {cmdBuf, shared_from_this()};
}

void CommandPool::recycle(VkCommandBuffer cmdBuf) {
  m_availableBuffers.push_back(cmdBuf);
}

CommandBuffer::~CommandBuffer() {
  if (m_commandBuffer != VK_NULL_HANDLE) {
    return;
  }
  if (auto pool = m_pool.lock()) {
    pool->recycle(m_commandBuffer);
  }
}
}  // namespace spoony::vkcore