#include "Buffer.hpp"

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

void Buffer::bindVertex(VkCommandBuffer cmdBuf) {
  VkDeviceSize offset{0};
  vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_buffer, &offset);
}

void Buffer::bindIndex(VkCommandBuffer cmdBuf) {
  vkCmdBindIndexBuffer(cmdBuf, m_buffer, 0, VK_INDEX_TYPE_UINT32);
}

Buffer::~Buffer() {
  vkDestroyBuffer(m_context.device(), m_buffer, nullptr);
  vkFreeMemory(m_context.device(), m_bufferMemory, nullptr);
}

}  // namespace spoony::vkcore