#include "VulkanUtils.hpp"

#include <array>
#include <set>
#include <unordered_set>

#include "Utils.hpp"

using namespace spoony::utils;

namespace spoony::vkcore::utils {

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                     VkSurfaceKHR surface) {
  QueueFamilyIndices indices;
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  for (int i = 0; i < queueFamilies.size(); ++i) {
    if (hasFlags(queueFamilies[i].queueFlags, VK_QUEUE_GRAPHICS_BIT)) {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    if (surface != VK_NULL_HANDLE)
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }
  }

  return indices;
}

bool checkLayerSupport(std::string_view layer) {
  static std::unordered_set<std::string> supportedLayerNames;
  if (supportedLayerNames.empty()) {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> supportedLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data());
    for (const auto& layer : supportedLayers) {
      supportedLayerNames.emplace(layer.layerName);
    }
  }
  return supportedLayerNames.contains(std::string(layer));
}

bool checkValidationLayerSupport() {
  bool support = false;
  for (const auto& requiredLayer : k_validationLayers) {
    support |= checkLayerSupport(requiredLayer);
  }

  return support;
}

bool checkExtensionSupport(std::string_view extension) {
  static std::unordered_set<std::string> supportedExtensionNames;
  if (supportedExtensionNames.empty()) {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                           supportedExtensions.data());
    for (const auto& ext : supportedExtensions) {
      supportedExtensionNames.emplace(ext.extensionName);
    }
  }
  return supportedExtensionNames.contains(std::string(extension));
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
  auto indices = findQueueFamilies(device, surface);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  // If no surface was passed, we assume this device does not
  // need to present to a display.
  bool needsToPresent = surface != VK_NULL_HANDLE;
  bool swapChainAdequate = false;

  if (extensionsSupported) {
    auto swapChainSupport = querySwapChainSupport(device, surface);
    swapChainAdequate = !swapChainSupport.formats.empty();
    if (needsToPresent)
      swapChainAdequate &= !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceFeatures supportedFeatures{};
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  bool presentRequirementsMet =
      !needsToPresent || indices.presentSupported() && swapChainAdequate;
  return indices.graphicsSupported() && presentRequirementsMet &&
         extensionsSupported && supportedFeatures.samplerAnisotropy;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       extensions.data());

  std::set<std::string> requiredExtensions(k_deviceExtensions.begin(),
                                           k_deviceExtensions.end());

  for (const auto& extension : extensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount > 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);
  if (presentModeCount > 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.presentModes.data());
  }
  return details;
}

VkImageView createImageView(VkDevice device,
                            VkImage image,
                            VkFormat format,
                            uint32_t mipLevels,
                            VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange{.aspectMask = aspectFlags,
                        .baseMipLevel = 0,
                        .levelCount = mipLevels,
                        .baseArrayLayer = 0,
                        .layerCount = 1}};

  VkImageView imageView;
  if (vkCreateImageView(device, &createInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create image view");
  }

  return imageView;
}

VkFormat findSupportedCandidates(std::span<VkFormat const> candidates,
                                 VkPhysicalDevice physicalDevice,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags features) {
  for (auto format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
    if (tiling == VK_IMAGE_TILING_LINEAR &&
        hasFlags(props.linearTilingFeatures, features)) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               hasFlags(props.optimalTilingFeatures, features)) {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format");
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
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

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
  return findSupportedCandidates(
      std::array{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                 VK_FORMAT_D24_UNORM_S8_UINT},
      physicalDevice, VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  // need to check both color and depth sample maximums
  VkSampleCountFlags counts =
      deviceProperties.limits.framebufferColorSampleCounts &
      deviceProperties.limits.framebufferDepthSampleCounts;

  for (int bits = 64; bits >= 1; bits /= 2) {
    if (hasFlags(counts, bits)) {
      return static_cast<VkSampleCountFlagBits>(bits);
    }
  }

  return VK_SAMPLE_COUNT_1_BIT;
}

constexpr bool hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
}
}  // namespace spoony::vkcore::utils