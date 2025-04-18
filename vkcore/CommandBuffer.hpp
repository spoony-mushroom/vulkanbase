#pragma once

#include "VulkanContext.hpp"

namespace spoony::vkcore {
class CommandBuffer {
 public:
  CommandBuffer(ContextHandle context);
  ~CommandBuffer();

 private:
  VkCommandBuffer m_commandBuffer;
  ContextHandle m_context;
  VkCommandPool m_pool;
};
}  // namespace spoony::vkcore