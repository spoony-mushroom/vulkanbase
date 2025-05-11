#pragma once

#include "Types.hpp"
#include "VulkanContext.hpp"
namespace spoony::vkcore {

class Buffer {
 public:
  Buffer() = default;
  Buffer(ContextHandle context,
         VkDeviceSize size,
         VkBufferUsageFlags usage,
         VkMemoryPropertyFlags properties);
  void bindVertex(VkCommandBuffer cmdBuf);
  void bindIndex(VkCommandBuffer cmdBuf);
  VkDeviceSize getSize() const { return m_size; };
  operator VkBuffer() const { return m_buffer; }

  virtual ~Buffer();

 protected:
  VkDevice getDevice() const { return m_context.device(); };
  VkDeviceMemory getMemory() const { return m_bufferMemory; };

 private:
  ContextHandle m_context;
  VkDeviceSize m_size;
  VkBuffer m_buffer;
  VkDeviceMemory m_bufferMemory;
};

class HostVisibleBuffer : public Buffer {
 public:
  HostVisibleBuffer() = default;
  HostVisibleBuffer(ContextHandle context,
                    VkDeviceSize size,
                    VkBufferUsageFlags usage)
      : Buffer(context,
               size,
               usage,
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {};

  HostVisibleBuffer(ContextHandle context,
                    std::ranges::contiguous_range auto&& srcData,
                    VkBufferUsageFlags usage)
      : HostVisibleBuffer(context,
                          sizeof(srcData.back()) * srcData.size(),
                          usage) {
    void* data;  // host (CPU) memory that is mapped to the buffer
    vkMapMemory(getDevice(), getMemory(), 0, getSize(), 0, &data);
    // copy the data
    memcpy(data, srcData.data(), getSize());
    vkUnmapMemory(getDevice(), getMemory());
  }
};

class MappedUniformBuffer : public HostVisibleBuffer {
 public:
  MappedUniformBuffer() = default;
  MappedUniformBuffer(ContextHandle context, size_t bufferSize)
      : HostVisibleBuffer(context,
                          static_cast<VkDeviceSize>(bufferSize),
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
    vkMapMemory(getDevice(), getMemory(), 0, getSize(), 0, &m_mappedData);
  }

  template <Blittable T>
  T& data() const {
    assert(sizeof(T) <= getSize());
    return *reinterpret_cast<T*>(m_mappedData);
  }

 private:
  void* m_mappedData;
};
}  // namespace spoony::vkcore