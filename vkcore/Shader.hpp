#pragma once

#include <span>
#include "vulkan/vulkan.h"

#include "VulkanContext.hpp"

namespace spoony::vkcore {
class Shader {
 public:
  Shader(ContextHandle context, std::span<const char> code);
  ~Shader();

  operator VkShaderModule() { return m_shaderModule; }

 private:
  ContextHandle m_context;
  VkShaderModule m_shaderModule;
};
}  // namespace spoony::vkcore