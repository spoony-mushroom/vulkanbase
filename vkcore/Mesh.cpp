#include "Mesh.hpp"

spoony::vkcore::Mesh::Mesh(const MeshData& data, const Renderer& renderer)
    : m_vertexBuffer(
          renderer.getContext(),
          data.getVertexBufferSizeBytes(),
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      m_indexBuffer(
          renderer.getContext(),
          data.getIndexBufferSizeBytes(),
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
  HostVisibleBuffer vertexStagingBuffer(renderer.getContext(), data.vertices,
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  renderer.copyBuffer(vertexStagingBuffer, m_vertexBuffer);

  HostVisibleBuffer indexStagingBuffer(renderer.getContext(), data.indices,
                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
                                       
  renderer.copyBuffer(indexStagingBuffer, m_indexBuffer);
}
