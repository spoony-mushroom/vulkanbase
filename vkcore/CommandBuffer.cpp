#include "CommandBuffer.hpp"

namespace spoony::vkcore {
CommandBuffer::CommandBuffer(ContextHandle context) : m_context(context) {
  m_pool = m_context.get()->getOrCreateCommandPool(std::this_thread::get_id());
  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_pool,
      .commandBufferCount = 1,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY};

  VK_CHECK(
      vkAllocateCommandBuffers(context.device(), &allocInfo, &m_commandBuffer),
      "allocate command buffer");
}

CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(m_context.device(), m_pool, 1, &m_commandBuffer);
}
}  // namespace spoony::vkcore