#include <array>
#include <format>
#include <unordered_set>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanContext.hpp"
#include "VulkanUtils.hpp"

using namespace spoony::utils;

namespace spoony::vkcore {

VulkanContextBuilder& VulkanContextBuilder::addExtension(
    std::string_view extension) {
  if (!checkExtensionSupport(extension)) {
    throw std::runtime_error(std::format(
        "The extension {} is not supported on this system", extension));
  }
  m_extensions.emplace_back(extension);
  return *this;
}

VulkanContextBuilder& VulkanContextBuilder::addLayer(std::string_view layer) {
  if (!checkLayerSupport(layer)) {
    throw std::runtime_error(
        std::format("The layer {} is not supported on this system", layer));
  }
  m_layers.emplace_back(layer);
  return *this;
}

VulkanContextBuilder& VulkanContextBuilder::setEnableValidation(bool enabled) {
  if (enabled && !checkValidationLayerSupport()) {
    throw std::runtime_error("Validation layers requested, but not supported");
  }
  m_enableValidationLayers = enabled;
  return *this;
}

ContextHandle VulkanContextBuilder::build() const {
  std::vector<const char*> extensionsCstr;
  std::transform(m_extensions.begin(), m_extensions.end(),
                 std::back_inserter(extensionsCstr),
                 [](const std::string& str) { return str.c_str(); });

  std::vector<const char*> layersCstr;
  std::transform(m_layers.begin(), m_layers.end(),
                 std::back_inserter(layersCstr),
                 [](const std::string& str) { return str.c_str(); });
  if (m_enableValidationLayers) {
    for (auto validationLayer : k_validationLayers) {
      layersCstr.push_back(validationLayer);
    }
  }

  ContextHandle context(std::make_shared<VulkanContext>(extensionsCstr, layersCstr));

  context.get()->initialize(m_surface);
  return context;
}

VulkanContextBuilder& VulkanContextBuilder::setSurface(VkSurfaceKHR surface) {
  m_surface = surface;
}

VulkanContext::VulkanContext(std::span<const char*> extensions,
                             std::span<const char*> layers) {
  createInstance(extensions, layers);
}

VulkanContext::~VulkanContext() {
  vkDestroyDevice(m_device, nullptr);
  vkDestroyInstance(m_instance, nullptr);
}

void VulkanContext::createInstance(std::span<const char*> extensions,
                                   std::span<const char*> layers) {
  VkApplicationInfo appInfo{.apiVersion = VK_API_VERSION_1_4};
  VkInstanceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
      .enabledLayerCount = static_cast<uint32_t>(layers.size()),
      .ppEnabledLayerNames = layers.data()};

  VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
  if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
    throw std::runtime_error("Incompatible driver");
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to create Vulkan instance");
  }
}

void VulkanContext::initialize(VkSurfaceKHR surface) {
  pickPhysicalDevice(surface);
  createLogicalDevice();
}

void VulkanContext::pickPhysicalDevice(VkSurfaceKHR surface) {
  uint32_t deviceCount;
  vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (isDeviceSuitable(device, surface)) {
      m_physicalDevice = device;
      break;
    }
  }

  if (m_physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("No suitable GPU found");
  }
}

VkSampleCountFlagBits VulkanContext::getMaxSampleCount() const {
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
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

void VulkanContext::createLogicalDevice() {}
}  // namespace spoony::vkcore