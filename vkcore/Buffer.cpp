#include "Buffer.hpp"

#include "VulkanUtils.hpp"
namespace spoony::vkcore {

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                               uint32_t typeFilter,
                               VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if (utils::hasBit(typeFilter, i) &&
        utils::hasFlags(memProperties.memoryTypes[i].propertyFlags,
                        properties)) {
      // found it
      return i;
    }
  }

  throw std::runtime_error("unable to find suitable memory type");
}

void createBuffer(ContextHandle context,
                  VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer& buffer,
                  VkDeviceMemory& memory) {
  VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                .size = size,
                                .usage = usage,
                                .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

  VK_CHECK(vkCreateBuffer(context.device(), &bufferInfo, nullptr, &buffer),
           "create  buffer");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(context.device(), buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(context.physicalDevice(),
                         memRequirements.memoryTypeBits, properties)};

  VK_CHECK(vkAllocateMemory(context.device(), &allocInfo, nullptr, &memory),
           "allocate buffer memory");

  vkBindBufferMemory(context.device(), buffer, memory, 0);
}
}  // namespace spoony::vkcore