#include "VulkanContext.hpp"

#include <array>
#include <format>
#include <set>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanUtils.hpp"
#include "WindowSurface.hpp"

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

#if defined __APPLE__ && defined __arm64__
  // required on Apple Silicon
  extensionsCstr.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensionsCstr.push_back(
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

  std::vector<const char*> layersCstr;
  std::transform(m_layers.begin(), m_layers.end(),
                 std::back_inserter(layersCstr),
                 [](const std::string& str) { return str.c_str(); });
  if (m_enableValidationLayers) {
    for (auto validationLayer : k_validationLayers) {
      layersCstr.push_back(validationLayer);
    }
  }

  return std::make_shared<VulkanContext>(extensionsCstr, layersCstr);
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
  createLogicalDevice(surface);
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

void VulkanContext::createLogicalDevice(VkSurfaceKHR surface) {

  auto indices = findQueueFamilies(m_physicalDevice, surface);

  VkPhysicalDeviceFeatures deviceFeatures{.samplerAnisotropy = VK_TRUE};

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies{indices.graphicsFamily.value(),
                                         indices.presentFamily.value()};
  float queuePriority = 1.f;
  for (auto queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(k_deviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = k_deviceExtensions.data();

  if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) !=
      VK_SUCCESS) {
    throw std::runtime_error("Unable to create logical device");
  }

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}
}  // namespace spoony::vkcore