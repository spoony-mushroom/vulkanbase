#pragma once

#include "CommandBuffer.hpp"

namespace spoony::vkcore {
class RenderPassModule {
 public:
  virtual ~RenderPassModule() = default;
  virtual void record(VkCommandBuffer cmdBuf, uint32_t imageIndex) = 0;

  private:
  VkExtent2D m_extents;
};
}  // namespace spoony::vkcore