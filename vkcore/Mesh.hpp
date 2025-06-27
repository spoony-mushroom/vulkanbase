#pragma once

#include "Buffer.hpp"

namespace spoony::vkcore {
class Mesh {
 public:
  Mesh(ContextHandle context, const MeshData& data);
  void draw(VkCommandBuffer cmdBuf);

 private:
  Buffer m_vertexBuffer;
  Buffer m_indexBuffer;
};
}  // namespace spoony::vkcore