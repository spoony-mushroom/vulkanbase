#pragma once
#include <vulkan/vulkan.h>
#include <span>

namespace spoony::vkcore {

class VulkanContext final {
 public:
  ~VulkanContext();
  VkInstance getInstance() { return m_instance; }
  VkDevice getDevice() const { return m_device; }
  VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }

  VkSampleCountFlagBits getMaxSampleCount() const;

 private:
  friend class VulkanContextBuilder;
  VulkanContext(std::span<const char*> extensions,
                std::span<const char*> layers);

  VkInstance m_instance{VK_NULL_HANDLE};
  VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
  VkDevice m_device{VK_NULL_HANDLE};

  VkQueue m_graphicsQueue;
  VkQueue m_presentQueue;

  void createInstance(std::span<const char*> extensions,
                      std::span<const char*> layers);
  void initialize(VkSurfaceKHR surface);
  void pickPhysicalDevice(VkSurfaceKHR surface);
  void createLogicalDevice();
};

class ContextHandle {
 public:
  explicit ContextHandle(std::shared_ptr<VulkanContext> contextPtr)
      : m_context(contextPtr) {}

  VkInstance instance() const { return m_context->getInstance(); }
  VkDevice device() const { return m_context->getDevice(); }
  VkPhysicalDevice physicalDevice() const {
    return m_context->getPhysicalDevice();
  }
  std::shared_ptr<VulkanContext> get() const { return m_context; }

 private:
  std::shared_ptr<VulkanContext> m_context;
};

class VulkanContextBuilder final {
 public:
  VulkanContextBuilder& addExtension(std::string_view extension);
  VulkanContextBuilder& addLayer(std::string_view layer);
  VulkanContextBuilder& setEnableValidation(bool enabled);
  VulkanContextBuilder& setSurface(VkSurfaceKHR surface);

  ContextHandle build() const;

 private:
  std::vector<std::string> m_extensions;
  std::vector<std::string> m_layers;
  bool m_enableValidationLayers{false};
  VkSurfaceKHR m_surface{VK_NULL_HANDLE};
};
}  // namespace spoony::vkcore