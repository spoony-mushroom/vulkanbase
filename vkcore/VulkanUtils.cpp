#include "VulkanUtils.hpp"
#include <unordered_set>

using namespace spoony::utils;

namespace spoony::vkcore {
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

static bool checkValidationLayerSupport() {
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
  bool swapChainAdequate = false;
  if (extensionsSupported) {
    auto swapChainSupport = querySwapChainSupport(device, surface);
    swapChainAdequate = !(swapChainSupport.formats.empty() ||
                          swapChainSupport.presentModes.empty());
  }

  VkPhysicalDeviceFeatures supportedFeatures{};
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  // If no surface was passed into, we assume this device does not
  // need to present to a display.
  bool presentRequirementsMet =
      surface == VK_NULL_HANDLE || indices.presentSupported();
  return indices.graphicsSupported() && presentRequirementsMet &&
         extensionsSupported && swapChainAdequate &&
         supportedFeatures.samplerAnisotropy;
}
}  // namespace spoony::vkcore