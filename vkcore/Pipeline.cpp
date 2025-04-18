#include "Pipeline.hpp"

#include <ranges>

#include "Shader.hpp"
#include "Types.hpp"
#include "VulkanUtils.hpp"

namespace spoony::vkcore {
Pipeline::Pipeline(ContextHandle context,
                   std::shared_ptr<RenderPass> renderPass,
                   int maxFramesInFlight)
    : m_context(context),
      m_renderPass(renderPass),
      k_maxFramesInFlight(maxFramesInFlight) {}

Pipeline::~Pipeline() {
  const auto device = m_context.device();
  vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);

  vkDestroyPipeline(device, m_pipeline, nullptr);
  vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
}

void Pipeline::initialize(
    VkVertexInputBindingDescription vertexBindingDescription,
    std::span<VkVertexInputAttributeDescription const>
        vertexAttributeDescriptions,
    std::span<VkPipelineShaderStageCreateInfo const> shaderStages) {
  assert(!shaderStages.empty());
  std::array dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = dynamicStates.size(),
      .pDynamicStates = dynamicStates.data(),
  };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertexBindingDescription,
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(vertexAttributeDescriptions.size()),
      .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()};

  VkPipelineInputAssemblyStateCreateInfo assemblyInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      // actually viewport and scissor will be set up later (dynamic)
      .scissorCount = 1,
      .viewportCount = 1};

  VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .lineWidth = 1.f,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      // because of our projection matrix flip, the windings are reversed
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0.f};

  VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable = VK_FALSE,
      .rasterizationSamples = m_renderPass->getSampleCount()};

  VkPipelineColorBlendAttachmentState colorBlendAttachment{
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_FALSE};

  // enable depth testing
  VkPipelineDepthStencilStateCreateInfo depthStencil{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      // fragment should be draw if its depth is LESS
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE};

  VkPipelineColorBlendStateCreateInfo colorBlending{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,  // Optional
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment};

  VkGraphicsPipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),

      .pDynamicState = &dynamicState,
      .pInputAssemblyState = &assemblyInfo,
      .pVertexInputState = &vertexInputInfo,
      .pRasterizationState = &rasterizer,
      .pViewportState = &viewportState,
      .pMultisampleState = &multisampling,
      .pColorBlendState = &colorBlending,
      .pDepthStencilState = &depthStencil,

      .layout = m_pipelineLayout,
      .renderPass = *m_renderPass,
      .subpass = 0};

  VK_CHECK(vkCreateGraphicsPipelines(m_context.device(),
                                     VK_NULL_HANDLE,  // pipelineCache
                                     1,               // createInfoCount
                                     &pipelineInfo, nullptr, &m_pipeline),
           "create graphics pipeline");
}

void Pipeline::createDescriptorSetLayout(
    std::span<const VkDescriptorSetLayoutBinding> bindings) {
  VkDescriptorSetLayoutCreateInfo layoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(bindings.size()),
      .pBindings = bindings.data()};

  VK_CHECK(vkCreateDescriptorSetLayout(m_context.device(), &layoutInfo, nullptr,
                                       &m_descriptorSetLayout),
           "create descriptor set layout");

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &m_descriptorSetLayout};

  VK_CHECK(vkCreatePipelineLayout(m_context.device(), &pipelineLayoutInfo,
                                  nullptr, &m_pipelineLayout),
           "create pipeline layout");
}

void Pipeline::createUniformBuffer(uint32_t binding, size_t size) {
  m_uniformBuffers.emplace(std::piecewise_construct,
                           std::forward_as_tuple(binding),
                           std::forward_as_tuple(m_context, size));
}

void Pipeline::createDescriptorPool() {
  auto poolSizes = std::to_array<VkDescriptorPoolSize>(
      {{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = k_maxFramesInFlight},
       {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = k_maxFramesInFlight}});

  VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = poolSizes.size(),
      .pPoolSizes = poolSizes.data(),
      .maxSets = k_maxFramesInFlight};

  VK_CHECK(vkCreateDescriptorPool(m_context.device(), &poolInfo, nullptr,
                                  &m_descriptorPool),
           "create descriptor pool");
}

void Pipeline::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(k_maxFramesInFlight,
                                             m_descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = m_descriptorPool,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data()};

  m_descriptorSets.resize(k_maxFramesInFlight);
  VK_CHECK(vkAllocateDescriptorSets(m_context.device(), &allocInfo,
                                    m_descriptorSets.data()),
           "allocate descriptor sets");
}

PipelineBuilder::PipelineBuilder(ContextHandle context,
                                 std::shared_ptr<RenderPass> renderPass)
    : m_context(context), m_renderPass(renderPass) {}

PipelineBuilder& PipelineBuilder::setShaders(
    std::span<const char> vertShaderCode,
    std::span<const char> fragShaderCode) {
  m_vertShader = std::make_unique<Shader>(m_context, vertShaderCode);
  m_fragShader = std::make_unique<Shader>(m_context, fragShaderCode);

  m_shaderStages.push_back(
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = *m_vertShader,
       .pName = "main"});

  m_shaderStages.push_back({
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = *m_fragShader,
      .pName = "main",
  });

  return *this;
}

PipelineBuilder& PipelineBuilder::addTextureSampler(uint32_t bindingIndex) {
  if (m_descriptorLayoutBindings.contains(bindingIndex)) {
    throw std::runtime_error("Another binding already exists at ");
  }

  m_descriptorLayoutBindings.emplace(
      bindingIndex,
      VkDescriptorSetLayoutBinding{
          .binding = bindingIndex,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImmutableSamplers = nullptr,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT});

  return *this;
}

std::unique_ptr<Pipeline> PipelineBuilder::create() const {
  assert(m_renderPass != nullptr);
  auto pipeline =
      std::make_unique<Pipeline>(m_context, m_renderPass, m_maxFramesInFlight);

  auto bindingsValues = std::views::values(m_descriptorLayoutBindings);
  std::vector<VkDescriptorSetLayoutBinding> layoutBindings{
      bindingsValues.begin(), bindingsValues.end()};

  pipeline->createDescriptorSetLayout(layoutBindings);
  pipeline->createDescriptorPool();
  pipeline->createDescriptorSets();
  pipeline->initialize(m_vertexBindingDescription,
                       m_vertexAttributeDescriptions, m_shaderStages);
  for (auto [binding, size] : m_uniformSizes) {
    pipeline->createUniformBuffer(binding, size);
  }

  return pipeline;
}
}  // namespace spoony::vkcore