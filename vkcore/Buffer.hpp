#pragma once

#include "Types.hpp"
#include "Utils.hpp"
#include "VulkanContext.hpp"

using namespace spoony::utils;

namespace spoony::vkcore {

inline VkBufferUsageFlags getExtraUsageFlags(
    VkMemoryPropertyFlags propertyFlags) {
  if (hasFlags(propertyFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
    return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  return 0;
}

class Buffer {
 public:
  Buffer() = default;
  Buffer(
      ContextHandle context,
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  Buffer(ContextHandle context,
         ContiguousSizedRange auto&& srcData,
         VkBufferUsageFlags usage,
         VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
      : Buffer(context,
               sizeInBytes(srcData),
               usage | getExtraUsageFlags(properties),
               properties) {
    if (hasFlags(properties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
      // The buffer we are creating is not visible to the host
      // We need to upload the data via a staging buffer
      Buffer stagingBuffer(context, getSize(), VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      stagingBuffer.copyHostData(srcData);
      // Perform a transfer
      copyFrom(stagingBuffer);
    } else if (hasFlags(properties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      copyHostData(srcData);
    } else {
      throw std::runtime_error("Failed to create buffer: Invalid memory property flags");
    }
  }
  Buffer(Buffer&& other);
  virtual ~Buffer();

  void bindVertex(VkCommandBuffer cmdBuf) const;
  void bindIndex(VkCommandBuffer cmdBuf, VkIndexType indexType) const;
  VkDeviceSize getSize() const { return m_size; };
  void copyFrom(const Buffer& src);

  operator VkBuffer() const { return m_buffer; }
  Buffer& operator=(Buffer&& other) noexcept;

 protected:
  VkDevice getDevice() const { return m_context.device(); };
  VkDeviceMemory getMemory() const { return m_bufferMemory; };
  void reset();

  void copyHostData(ContiguousSizedRange auto&& srcData) {
    assert(getSize() >= sizeInBytes(srcData));
    void* data;  // host (CPU) memory that is mapped to the buffer
    vkMapMemory(getDevice(), getMemory(), 0, getSize(), 0, &data);
    // copy the data
    memcpy(data, srcData.data(), getSize());
    vkUnmapMemory(getDevice(), getMemory());
  }

 private:
  ContextHandle m_context;
  VkDeviceSize m_size{};
  VkBuffer m_buffer{};
  VkDeviceMemory m_bufferMemory{};
};

class MappedUniformBuffer : public Buffer {
 public:
  MappedUniformBuffer() = default;
  MappedUniformBuffer(ContextHandle context, size_t bufferSize);

  template <Blittable T>
  T& data() const {
    assert(sizeof(T) <= getSize());
    return *reinterpret_cast<T*>(m_mappedData);
  }

 private:
  void* m_mappedData;
};

}  // namespace spoony::vkcore