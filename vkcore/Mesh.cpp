#include "Mesh.hpp"

spoony::vkcore::Mesh::Mesh(ContextHandle context, const MeshData& data)
    : m_vertexBuffer(context, data.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
      m_indexBuffer(context, data.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {}

void spoony::vkcore::Mesh::draw(VkCommandBuffer cmdBuf) {
  m_vertexBuffer.bindVertex(cmdBuf);
  m_indexBuffer.bindIndex(cmdBuf, MeshData::k_indexType);
  auto numIndices = m_indexBuffer.getSize() / sizeof(MeshData::IndexType);
  vkCmdDrawIndexed(cmdBuf, numIndices, 1, 0, 0, 0);
}
