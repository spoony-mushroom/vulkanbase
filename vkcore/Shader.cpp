#include "Shader.hpp"

#include "VulkanUtils.hpp"

namespace spoony::vkcore {
Shader::Shader(ContextHandle context, std::span<const char> code)
    : m_context(context) {
  VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = code.size(),
      .pCode = reinterpret_cast<const uint32_t*>(code.data())};

  VK_CHECK(vkCreateShaderModule(m_context.device(), &createInfo, nullptr, &m_shaderModule),
           "creating shader module");
}
Shader::~Shader() {
    vkDestroyShaderModule(m_context.device(), m_shaderModule, nullptr);
}
}  // namespace spoony::vkcore