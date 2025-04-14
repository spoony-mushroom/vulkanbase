#pragma once

#include "Types.hpp"
#include "VulkanContext.hpp"
namespace spoony::vkcore {

void createBuffer(ContextHandle context,
                  VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer& buffer,
                  VkDeviceMemory& memory);

class MappedUniformBuffer {
 public:
  MappedUniformBuffer(ContextHandle context, size_t bufferSize)
      : m_context(context), m_size(bufferSize) {
    createBuffer(m_context, m_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 m_buffer, m_bufferMemory);
    vkMapMemory(m_context.device(), m_bufferMemory, 0, m_size, 0,
                &m_mappedData);
  }
  ~MappedUniformBuffer() {
    vkDestroyBuffer(m_context.device(), m_buffer, nullptr);
    vkFreeMemory(m_context.device(), m_bufferMemory, nullptr);
  }

  template <typename T>
  T& data() const {
    assert(sizeof(T) <= m_size);
    return *reinterpret_cast<T*>(m_mappedData);
  }

 private:
  size_t m_size;
  ContextHandle m_context;
  VkBuffer m_buffer;
  VkDeviceMemory m_bufferMemory;
  void* m_mappedData;
};
}  // namespace spoony::vkcore