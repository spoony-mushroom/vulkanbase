#include "RenderPass.hpp"

#include "VulkanUtils.hpp"

namespace spoony::vkcore {
spoony::vkcore::RenderPass::RenderPass(ContextHandle context,
                                       const RenderPassConfig& config)
    : m_context(context) {
  m_sampleCount = config.msaaSamples;
  // Attachment 0: color
  VkAttachmentDescription colorAttachment{
      .format = config.colorImageFormat,
      .samples = config.msaaSamples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentReference colorAttachmentRef{
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  // Attachment 1: depth
  VkAttachmentDescription depthAttachment{
      .format = config.depthStencilImageFormat,
      .samples = config.msaaSamples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      // contents not needed after rendering
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentReference depthAttachmentRef{
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  // Attachment 2: Presentation (swap chain image)
  // multisampled color attachments must be resolved to a "regular" image
  // for presentation
  VkAttachmentDescription colorResolveAttachment{
      .format = config.colorImageFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

  VkAttachmentReference colorResolveAttachmentRef{
      .attachment = 2, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef,
      .pDepthStencilAttachment = &depthAttachmentRef,
      .pResolveAttachments = &colorResolveAttachmentRef};

  std::array attachments{colorAttachment, depthAttachment,
                         colorResolveAttachment};
  VkRenderPassCreateInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = attachments.size(),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
  };

  // This part is a bit confusing and I'm not sure if I understand it
  // completely.
  // We're using Vulkan's automatic layout transition, which happens if it
  // has an initial layout of VK_IMAGE_LAYOUT_UNDEFINED. There is an implicit
  // subpass (SUBPASS_EXTERNAL) that handles the image layout transition.
  // For correctness, we need to wait for the transition before we can write
  // to the image attachment. We do not need to do this if we instead handle
  // the layout transitions explicitly ourselves.
  VkSubpassDependency dependency{};
  // implicit subpass before our subpass
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;  // index of our subpass

  // Source pass: implicit transition
  // stage(s) to wait on
  // The confusing bit is that these are not necessarily the the stages of
  // the external subpass that we're waiting on. It's set this way so that
  // we form a dependency chain that is [acquisition]->[layout]->[render],
  // were [acquisition] signals a semaphore set to the COLOR_ATTACHMENT_OUTPUT
  // stage. At least this is my understanding. This is poorly-documented in
  // the Vulkan literature.
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  // We're not accessing the memory involved in this subpass
  dependency.srcAccessMask = 0;

  // Wait for the src pass to finish before executing the
  // STAGE_COLOR_ATTACHMENT_OUTPUT and
  // STAGE_EARLY_FRAGMENT_TESTS stages
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  // indicates that our subpass needs to be able to write to the attachments
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VK_CHECK(vkCreateRenderPass(m_context.device(), &renderPassInfo, nullptr,
                              &m_renderPass),
           "create render pass");
}

RenderPass::~RenderPass() {
  vkDestroyRenderPass(m_context.device(), m_renderPass, nullptr);
}
}  // namespace spoony::vkcore