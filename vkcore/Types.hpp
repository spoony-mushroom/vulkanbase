#pragma once

#include <concepts>
#include <type_traits>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.h>

namespace spoony::vkcore {
template <typename T>
concept Blittable =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

template <typename R, typename T>
concept ContiguousRangeOf =
    std::ranges::contiguous_range<R> &&
    std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, T>;

template <typename T>
concept HasBindingDescriptions =
    requires(std::vector<VkVertexInputAttributeDescription> vec) {
      {
        T::getBindingDescription()
      } -> std::convertible_to<VkVertexInputBindingDescription>;
      vec = T::getAttributeDescriptions();
    };

template <typename T>
concept VertexBindable = HasBindingDescriptions<T> && Blittable<T>;

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static constexpr auto getBindingDescription() {
    return VkVertexInputBindingDescription{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
  }

  static constexpr auto getAttributeDescriptions() {
    return std::vector<VkVertexInputAttributeDescription>{
        {.binding = 0,
         .location = 0,  // where to find this attribute in the vertex shader
         .format = VK_FORMAT_R32G32B32_SFLOAT,
         .offset = offsetof(Vertex, pos)},
        {.binding = 0,
         .location = 1,
         .format = VK_FORMAT_R32G32B32_SFLOAT,
         .offset = offsetof(Vertex, color)},
        {.binding = 0,
         .location = 2,
         .format = VK_FORMAT_R32G32_SFLOAT,
         .offset = offsetof(Vertex, texCoord)}};
  }

  bool operator==(const Vertex& other) const = default;
};

static void hash_combine(size_t& seed) {};

template <typename T, typename... Rest>
inline void hash_combine(size_t& seed, const T& v, const Rest&... rest) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  hash_combine(seed, rest...);
}

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};
}  // namespace spoony::vkcore

namespace std {
template <>
struct hash<spoony::vkcore::Vertex> {
  size_t operator()(const spoony::vkcore::Vertex& v) const {
    size_t seed = 0;
    spoony::vkcore::hash_combine(seed, v.pos, v.color, v.texCoord);
    return seed;
  }
};
}  // namespace std