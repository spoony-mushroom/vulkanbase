#pragma once

#include "Buffer.hpp"
#include "Renderer.hpp"

namespace spoony::vkcore {
class Mesh {
 public:
  Mesh(const MeshData& data, const Renderer& renderer);

 private:
  Buffer m_vertexBuffer;
  Buffer m_indexBuffer;
};
}  // namespace spoony::vkcore