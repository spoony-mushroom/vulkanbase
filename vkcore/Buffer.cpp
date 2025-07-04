#include "Buffer.hpp"

#include "CommandBuffer.hpp"
#include "Utils.hpp"
#include "VulkanUtils.hpp"

namespace spoony::vkcore {

using namespace spoony::utils;

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                               uint32_t typeFilter,
                               VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if (hasBit(typeFilter, i) &&
        hasFlags(memProperties.memoryTypes[i].propertyFlags, properties)) {
      // found it
      return i;
    }
  }

  throw std::runtime_error("unable to find suitable memory type");
}

Buffer::Buffer(ContextHandle context,
               VkDeviceSize size,
               VkBufferUsageFlags usage,
               VkMemoryPropertyFlags properties)
    : m_context(context), m_size(size) {
  VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                .size = size,
                                .usage = usage,
                                .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

  VK_CHECK(vkCreateBuffer(context.device(), &bufferInfo, nullptr, &m_buffer),
           "create  buffer");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(context.device(), m_buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(context.physicalDevice(),
                         memRequirements.memoryTypeBits, properties)};

  VK_CHECK(
      vkAllocateMemory(context.device(), &allocInfo, nullptr, &m_bufferMemory),
      "allocate buffer memory");

  vkBindBufferMemory(context.device(), m_buffer, m_bufferMemory, 0);
}

Buffer::Buffer(Buffer&& other) {
  m_buffer = other.m_buffer;
  m_bufferMemory = other.m_bufferMemory;
  m_context = std::move(other.m_context);
  m_size = other.m_size;

  other.m_buffer = VK_NULL_HANDLE;
  other.m_bufferMemory = VK_NULL_HANDLE;
  other.m_size = 0;
}

Buffer& Buffer::operator=(Buffer&& other)  noexcept {
  reset();
  m_buffer = other.m_buffer;
  m_bufferMemory = other.m_bufferMemory;
  m_context = std::move(other.m_context);
  m_size = other.m_size;

  other.m_buffer = VK_NULL_HANDLE;
  other.m_bufferMemory = VK_NULL_HANDLE;
  other.m_size = 0;
  return *this;
}

void Buffer::bindVertex(VkCommandBuffer cmdBuf) const {
  VkDeviceSize offset{0};
  vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_buffer, &offset);
}

void Buffer::bindIndex(VkCommandBuffer cmdBuf, VkIndexType indexType) const {
  vkCmdBindIndexBuffer(cmdBuf, m_buffer, 0, indexType);
}

void Buffer::copyFrom(const Buffer& src) {
  auto scope = AutoSubmitCommandBuffer(m_context);

  VkBufferCopy copyRegion{.size = src.getSize()};
  vkCmdCopyBuffer(scope, src, m_buffer, 1, &copyRegion);
}

void Buffer::reset() {
  if (m_buffer == VK_NULL_HANDLE) {
    return;
  }

  vkDestroyBuffer(m_context.device(), m_buffer, nullptr);
  vkFreeMemory(m_context.device(), m_bufferMemory, nullptr);
}

Buffer::~Buffer() {
  reset();
}

MappedUniformBuffer::MappedUniformBuffer(ContextHandle context,
                                         size_t bufferSize)
    : Buffer(context,
             static_cast<VkDeviceSize>(bufferSize),
             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
  vkMapMemory(getDevice(), getMemory(), 0, getSize(), 0, &m_mappedData);
}

}  // namespace spoony::vkcore