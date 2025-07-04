#pragma once

#include "Buffer.hpp"

namespace spoony::vkcore {
class Mesh {
 public:
  Mesh(ContextHandle context, const MeshData& data);
  Mesh(const Mesh& mesh) = delete;
  Mesh(Mesh&& other) noexcept = default;
  Mesh& operator=(const Mesh& mesh) = delete;
  Mesh& operator=(Mesh&& other) noexcept = default;
  void draw(VkCommandBuffer cmdBuf) const;

 private:
  Buffer m_vertexBuffer;
  Buffer m_indexBuffer;
};
}  // namespace spoony::vkcore