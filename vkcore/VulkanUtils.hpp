#pragma once

#include <concepts>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include <vulkan/vulkan.h>

#define VK_CHECK(result, msg)                             \
  do {                                                    \
    if ((result) != VK_SUCCESS) {                         \
      throw std::runtime_error("Operation failed: " msg); \
    }                                                     \
  } while (0)

namespace spoony::utils {
template <typename T>
concept Enum = std::is_enum_v<T>;

inline constexpr bool hasFlags(std::integral auto value,
                               std::integral auto flagsToCheck) {
  return (value & flagsToCheck) == flagsToCheck;
}

template <std::integral T, Enum E>
inline constexpr bool hasFlags(T value, E flagsToCheck) {
  auto flagsValue = static_cast<std::underlying_type_t<E>>(flagsToCheck);
  return (value & flagsValue) == flagsValue;
}

inline constexpr bool hasBit(std::convertible_to<size_t> auto value,
                             std::integral auto index) {
  return value & (1 << index);
}
}  // namespace spoony::utils

namespace spoony::vkcore {

constexpr std::array k_validationLayers{"VK_LAYER_KHRONOS_validation"};

constexpr std::array k_deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(__APPLE__) && defined(__arm64__)
                                        "VK_KHR_portability_subset"
#endif
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool presentSupported() const { return presentFamily.has_value(); }
  bool graphicsSupported() const { return graphicsFamily.has_value(); }

  bool isComplete() const { return presentSupported() && graphicsSupported(); }
};

bool checkLayerSupport(std::string_view layer);
bool checkValidationLayerSupport();
bool checkExtensionSupport(std::string_view extension);
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                     VkSurfaceKHR surface);

VkImageView createImageView(VkDevice device,
                            VkImage image,
                            VkFormat format,
                            uint32_t mipLevels,
                            VkImageAspectFlags aspectFlags);
VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
VkFormat findSupportedCandidates(std::span<VkFormat const> candidates,
                                 VkPhysicalDevice physicalDevice,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags features);
}  // namespace spoony::vkcore
