#include <cstdlib>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vkcore/Renderer.hpp"
using namespace spoony;

namespace spoony::vkcore {
    class Renderer;
}

class TestApp {
 public:
  TestApp() = default;

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Hello World", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
    // glfwSetFramebufferSizeCallback(m_window, frameBufferResizeCallback);
  };

  void initGraphics() { m_Renderer = std::make_unique<vkcore::Renderer>(m_window); }

  void mainLoop() {
    while (!glfwWindowShouldClose(m_window)) {
      glfwPollEvents();
    //   drawFrame();
    }

    // vkDeviceWaitIdle(device);
  }

 private:
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  GLFWwindow* m_window;
  std::unique_ptr<vkcore::Renderer> m_Renderer;

  static void frameBufferResizeCallback(GLFWwindow* window,
                                        int width,
                                        int height) {
    throw std::runtime_error("Not implemented yet");
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