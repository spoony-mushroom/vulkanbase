#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.h>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static constexpr auto getBindingDescription() {
        return VkVertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    static constexpr auto getAttributeDescriptions() {
        return std::array {
            VkVertexInputAttributeDescription {
                .binding = 0,
                .location = 0,  // where to find this attribute in the vertex shader
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, pos)
            },
            VkVertexInputAttributeDescription {
                .binding = 0,
                .location = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, color)
            },
            VkVertexInputAttributeDescription{
                .binding = 0,
                .location = 2,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texCoord)
            }
        };
    }

    bool operator==(const Vertex& other) const = default;
};

void hash_combine(size_t& seed) {};

template<typename T, typename... Rest>
inline void hash_combine(size_t& seed, const T& v, const Rest&... rest) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash_combine(seed, rest...);
}

namespace std {
    template<>
    struct hash<Vertex> {
        size_t operator()(const Vertex& v) const{
            size_t seed = 0;
            hash_combine(seed, v.pos, v.color, v.texCoord);
            return seed;
        }
    };
}

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};