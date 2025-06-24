#include "CommandBuffer.hpp"

namespace spoony::vkcore {

class CommandPool final : public std::enable_shared_from_this<CommandPool> {
 public:
  CommandPool(ContextHandle context, uint32_t queue);
  CommandPool(ContextHandle context);
  ~CommandPool();
  CommandBuffer acquire();
  void recycle(VkCommandBuffer cmdBuf);

 private:
  friend class CommandBuffer;
  static CommandPool& getInstance(ContextHandle context);
  VkCommandPool m_pool;
  ContextHandle m_context;
};

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

CommandPool::CommandPool(ContextHandle context)
    : CommandPool(
          context,
          context.get()->getQueueFamilyIndices().graphicsFamily.value()) {}

CommandPool::~CommandPool() {
  vkDestroyCommandPool(m_context.device(), m_pool, nullptr);
}

CommandBuffer CommandPool::acquire() {
  VkCommandBuffer cmdBuf{VK_NULL_HANDLE};
  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_pool,
      .commandBufferCount = 1,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY};

  VK_CHECK(vkAllocateCommandBuffers(m_context.device(), &allocInfo, &cmdBuf),
           "allocate command buffer");
  return {cmdBuf, shared_from_this()};
}

void CommandPool::recycle(VkCommandBuffer cmdBuf) {
  vkFreeCommandBuffers(m_context.device(), m_pool, 1, &cmdBuf);
}

CommandPool& CommandPool::getInstance(ContextHandle context) {
  // Each command buffer can only be used by the thread that create it.
  // To avoid sync overhead, each thread gets its own command pool instance.
  static thread_local std::unordered_map<VkInstance,
                                         std::shared_ptr<CommandPool>>
      instances;
  auto [it, inserted] = instances.try_emplace(
      context.instance(), std::make_shared<CommandPool>(context));
  return *it->second;
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept {
  m_commandBuffer = other.m_commandBuffer;
  m_pool = std::move(other.m_pool);

  other.m_commandBuffer = VK_NULL_HANDLE;
}

CommandBuffer::~CommandBuffer() {
  if (m_commandBuffer != VK_NULL_HANDLE) {
    return;
  }
  if (auto pool = m_pool.lock()) {
    pool->recycle(m_commandBuffer);
  }
}
CommandBuffer CommandBuffer::acquire(ContextHandle context) {
  return CommandPool::getInstance(context).acquire();
}
}  // namespace spoony::vkcore