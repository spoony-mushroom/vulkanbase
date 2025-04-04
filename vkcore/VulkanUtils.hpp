#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>
#include <vector>

#include <vulkan/vulkan.h>

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

  bool isComplete() const {
    return presentSupported && graphicsSupported;
  }
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
}  // namespace spoony::vkcore
