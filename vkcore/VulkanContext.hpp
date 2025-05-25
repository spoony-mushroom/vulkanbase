#pragma once
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <thread>

#include <vulkan/vulkan.h>

#include "VulkanUtils.hpp"

namespace spoony::vkcore {

class VulkanContext final {
 public:
  VulkanContext(std::span<const char*> extensions,
                std::span<const char*> layers);
  ~VulkanContext();
  void initialize(VkSurfaceKHR surface);

  VkInstance getInstance() { return m_instance; }
  VkDevice getDevice() const { return m_device; }
  VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
  VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
  VkQueue getPresentQueue() const { return m_presentQueue; }

  VkSampleCountFlagBits getMaxSampleCount() const;
  void deviceWaitIdle() const { vkDeviceWaitIdle(m_device); };

 private:
  friend class VulkanContextBuilder;

  VkInstance m_instance{VK_NULL_HANDLE};
  VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
  VkDevice m_device{VK_NULL_HANDLE};

  VkQueue m_graphicsQueue;
  VkQueue m_presentQueue;

  void createInstance(std::span<const char*> extensions,
                      std::span<const char*> layers);

  void pickPhysicalDevice(VkSurfaceKHR surface);
  void createLogicalDevice(VkSurfaceKHR surface);
};

class ContextHandle {
 public:
  ContextHandle() = default;
  ContextHandle(std::shared_ptr<VulkanContext> contextPtr)
      : m_context(contextPtr) {}
  ContextHandle(const ContextHandle&) = default;
  ContextHandle(ContextHandle&&) = default;

  void initialize(VkSurfaceKHR surface) { m_context->initialize(surface); }
  VkInstance instance() const { return m_context->getInstance(); }
  VkDevice device() const { return m_context->getDevice(); }
  VkPhysicalDevice physicalDevice() const {
    return m_context->getPhysicalDevice();
  }
  std::shared_ptr<VulkanContext> get() const { return m_context; }

  bool operator==(std::nullptr_t) const noexcept {
    return m_context == nullptr;
  }

  bool operator!=(std::nullptr_t) const noexcept {
    return m_context != nullptr;
  }

  ContextHandle& operator=(ContextHandle&& other) {
    m_context = std::move(other.m_context);
    return *this;
  }

 private:
  std::shared_ptr<VulkanContext> m_context;
};

class VulkanContextBuilder final {
 public:
  VulkanContextBuilder& addExtension(std::string_view extension);
  VulkanContextBuilder& addLayer(std::string_view layer);
  VulkanContextBuilder& setEnableValidation(bool enabled);

  ContextHandle build() const;

 private:
  std::vector<std::string> m_extensions;
  std::vector<std::string> m_layers;
  bool m_enableValidationLayers{false};
};
}  // namespace spoony::vkcore