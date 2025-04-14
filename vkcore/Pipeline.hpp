#pragma once
#include <vulkan/vulkan.h>
#include <map>
#include <memory>

#include "Buffer.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "Types.hpp"

namespace spoony::vkcore {

struct PipelineConfig {
  uint32_t width;
  uint32_t height;
  uint32_t maxFramesInFlight;
};

class Pipeline {
 public:
  Pipeline(ContextHandle context,
           std::shared_ptr<RenderPass> renderPass,
           int maxFramesInFlight);
  ~Pipeline();

 private:
  const uint32_t k_maxFramesInFlight;
  ContextHandle m_context;
  VkPipeline m_pipeline;
  VkPipelineLayout m_pipelineLayout;

  VkDescriptorSetLayout m_descriptorSetLayout;
  VkDescriptorPool m_descriptorPool;
  std::vector<VkDescriptorSet> m_descriptorSets;
  std::vector<MappedUniformBuffer> m_uniformBuffers;

  std::shared_ptr<RenderPass> m_renderPass;

  void createDescriptorSetLayout(
      std::span<const VkDescriptorSetLayoutBinding> bindings);
  void createDescriptorPool();
  void createDescriptorSets();
  void initialize(
      VkVertexInputBindingDescription vertexBindingDescription,
      std::span<VkVertexInputAttributeDescription const>
          vertexAttributeDescriptions,
      std::span<VkPipelineShaderStageCreateInfo const> shaderStages);

  friend class PipelineBuilder;
};

class PipelineBuilder {
 public:
  PipelineBuilder(ContextHandle context, std::shared_ptr<RenderPass> renderPass);

  PipelineBuilder& setMaxFramesInFlight(int maxFramesInFlight) {
    m_maxFramesInFlight = maxFramesInFlight;
    return *this;
  }

  template <VertexBindable T>
  PipelineBuilder& setVertexType() {
    m_vertexBindingDescription = T::getBindingDescription();
    m_vertexAttributeDescriptions = T::getAttributeDescriptions();
    return *this;
  }

  PipelineBuilder& setShaders(std::span<const char> vertShaderCode,
                              std::span<const char> fragShaderCode);

  template <Blittable T>
  PipelineBuilder& addVertShaderUniform(uint32_t bindingIndex) {
    if (m_descriptorLayoutBindings.contains(bindingIndex)) {
      throw std::runtime_error("Another binding already exists at ");
    }
    m_descriptorLayoutBindings.emplace(
        bindingIndex, VkDescriptorSetLayoutBinding{
                          .binding = bindingIndex,
                          .descriptorCount = 1,
                          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT});
    m_uniformSizes[bindingIndex] = sizeof(T);
    return *this;
  }

  PipelineBuilder& addTextureSampler(uint32_t bindingIndex);

  bool validate() const { return true; }  // TODO

  std::unique_ptr<Pipeline> create();

 private:
  int m_maxFramesInFlight{2};
  ContextHandle m_context;
  std::shared_ptr<RenderPass> m_renderPass;
  std::unique_ptr<Shader> m_vertShader;
  std::unique_ptr<Shader> m_fragShader;
  std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
  VkVertexInputBindingDescription m_vertexBindingDescription;
  std::vector<VkVertexInputAttributeDescription> m_vertexAttributeDescriptions;
  std::map<uint32_t, VkDescriptorSetLayoutBinding> m_descriptorLayoutBindings;
  std::map<uint32_t, size_t> m_uniformSizes;
  VkViewport m_viewPort;
};
}  // namespace spoony::vkcore