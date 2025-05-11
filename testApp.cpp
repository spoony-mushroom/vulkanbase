#include <cstdlib>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vkcore/Mesh.hpp"
#include "vkcore/Renderer.hpp"
#include "vkcore/Types.hpp"
#include "vkcore/UnlitRenderPass.hpp"
using namespace spoony;

class TestApp {
 public:
  TestApp() = default;

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Hello World", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
  };

  void initGraphics() {
    m_renderer = std::make_unique<vkcore::Renderer>(m_window);
    m_renderPass = &m_renderer->addRenderPassModule<vkcore::UnlitRenderPass>();
  }

  void mainLoop() {
    m_renderer->registerCurrentThread();
    initMeshes();
    while (!glfwWindowShouldClose(m_window)) {
      glfwPollEvents();
      // m_renderer->drawFrame();
    }
  }

 private:
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  GLFWwindow* m_window;
  std::unique_ptr<vkcore::Renderer> m_renderer;
  vkcore::UnlitRenderPass* m_renderPass;

  std::unique_ptr<vkcore::Mesh> m_mesh;

  static void frameBufferResizeCallback(GLFWwindow* window,
                                        int width,
                                        int height) {
    throw std::runtime_error("Not implemented yet");
  }

  void initMeshes() {
    static const vkcore::MeshData meshData{
      .vertices = {
        {.pos{-0.5f, -0.5f, 0.f}, .color{1.f, 0, 0}, .texCoord{0, 1.f}},
        {.pos{0.5f, -0.5f, 0.f}, .color{0, 1.f, 0}, .texCoord{1.f, 1.f}},
        {.pos{0.5f, 0.5f, 0.f}, .color{0, 0, 1.f}, .texCoord{1.f, 0}},
        {.pos{-0.5f, 0.5f, 0.f}, .color{1.f, 1.f, 1.f}, .texCoord{0, 0}},

        {.pos{-0.5f, -0.5f, -0.5f}, .color{1.f, 0, 0}, .texCoord{0, 1.f}},
        {.pos{0.5f, -0.5f, -0.5f}, .color{0, 1.f, 0}, .texCoord{1.f, 1.f}},
        {.pos{0.5f, 0.5f, -0.5f}, .color{0, 0, 1.f}, .texCoord{1.f, 0}},
        {.pos{-0.5f, 0.5f, -0.5f}, .color{1.f, 1.f, 1.f}, .texCoord{0, 0}}},
        .indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4}
    };

    m_mesh = std::make_unique<vkcore::Mesh>(meshData, *m_renderer);
  }

  void updateCamera() {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    auto renderExtent = m_renderer->getExtent();
    // rotate the model around the z-axis at 90 degrees/s
    auto model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f),
                             glm::vec3(0, 0, 1.f));
    auto view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0, 0, 0),
                            glm::vec3(0, 0, 1.f));  // up is +z
    auto proj = glm::perspective(
        glm::radians(45.f), renderExtent.width / (float)renderExtent.height,
        0.1f, 10.f);

    // switch from OpenGL convention for clip coordinates (y up) to Vulkan
    // convention (y down) invert the y scaling factor in the projection matrix
    proj[1][1] *= -1;

    m_renderPass->setModelViewProjection(model, view, proj);
  }
};

int main() {
  try {
    TestApp app;
    app.initWindow();
    app.initGraphics();
    app.mainLoop();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}