#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>

#include "types.hpp"
#include "utils.hpp"

class HelloTriangleApplication {
 public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

 private:
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  static constexpr char MODEL_PATH[]{"resources/viking_room.obj"};
  static constexpr char TEXTURE_PATH[]{"resources/viking_room.png"};

  // The number of frames that we can draw to at the same time.
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  static constexpr char VERT_SHADER_SPV[]{"shaders/triangle_app_vert.spv"};
  static constexpr char FRAG_SHADER_SPV[]{"shaders/triangle_app_frag.spv"};

  const std::vector<const char*> validationLayers{
      "VK_LAYER_KHRONOS_validation"};

  const std::vector<const char*> deviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(__APPLE__) && defined(__arm64__)
      "VK_KHR_portability_subset"
#endif
  };

#if NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

  GLFWwindow* window;
  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  VkRenderPass renderPass;
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkCommandPool commandPool;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  // For each frame we want to draw to, we need a separate command buffer and
  // synchronization objects
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  std::vector<VkFramebuffer> swapChainFramebuffers;

  bool framebufferResized = false;
  uint32_t currentFrame = 0;

  // const std::vector<Vertex> vertices{
  //     {.pos{-0.5f, -0.5f, 0.f}, .color{1.f, 0, 0}, .texCoord{0, 1.f}},
  //     {.pos{0.5f, -0.5f, 0.f}, .color{0, 1.f, 0}, .texCoord{1.f, 1.f}},
  //     {.pos{0.5f, 0.5f, 0.f}, .color{0, 0, 1.f}, .texCoord{1.f, 0}},
  //     {.pos{-0.5f, 0.5f, 0.f}, .color{1.f, 1.f, 1.f}, .texCoord{0, 0}},

  //     {.pos{-0.5f, -0.5f, -0.5f}, .color{1.f, 0, 0}, .texCoord{0, 1.f}},
  //     {.pos{0.5f, -0.5f, -0.5f}, .color{0, 1.f, 0}, .texCoord{1.f, 1.f}},
  //     {.pos{0.5f, 0.5f, -0.5f}, .color{0, 0, 1.f}, .texCoord{1.f, 0}},
  //     {.pos{-0.5f, 0.5f, -0.5f}, .color{1.f, 1.f, 1.f}, .texCoord{0, 0}}};

  // // limited to 65535 vertices
  // const std::vector<uint16_t> indices{0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;

  uint32_t mipLevels;
  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;

  // The color and depth buffers that we'll be performing the rendering into
  VkImage colorImage;
  VkDeviceMemory colorImageMemory;
  VkImageView colorImageView;

  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;

  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;

  VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  class ScopedCommandBuffer {
   public:
    ScopedCommandBuffer(VkDevice device, VkCommandPool pool, VkQueue queue)
        : device(device), commandPool(pool), queue(queue) {
      VkCommandBufferAllocateInfo allocInfo{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandBufferCount = 1,
          .commandPool = commandPool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY};

      vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

      VkCommandBufferBeginInfo beingInfo{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

      vkBeginCommandBuffer(commandBuffer, &beingInfo);
    }
    ~ScopedCommandBuffer() {
      vkEndCommandBuffer(commandBuffer);

      VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                              .commandBufferCount = 1,
                              .pCommandBuffers = &commandBuffer};

      vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
      vkQueueWaitIdle(queue);

      vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    operator VkCommandBuffer() const noexcept { return commandBuffer; }

   private:
    VkDevice device;
    VkCommandBuffer commandBuffer;
    VkCommandPool commandPool;
    VkQueue queue;
  };

  static void frameBufferResizeCallback(GLFWwindow* window,
                                        int width,
                                        int height) {
    void* userPtr = glfwGetWindowUserPointer(window);
    auto* app = reinterpret_cast<HelloTriangleApplication*>(userPtr);
    app->framebufferResized = true;
  }

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window =
        glfwCreateWindow(WIDTH, HEIGHT, "Hello Triangle", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
  }

  bool checkValidationLayerSupport() const {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    for (const auto& requiredLayer : validationLayers) {
      if (std::find_if(
              layers.begin(), layers.end(), [requiredLayer](const auto& layer) {
                return std::strcmp(layer.layerName, requiredLayer) == 0;
              }) == layers.end()) {
        return false;
      }
    }

    return true;
  }

  void createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
      throw std::runtime_error(
          "Validation layers requested, but not supported");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                           extensions.data());

    std::cout << "available extensions:\n";
    for (const auto& ext : extensions) {
      std::cout << ext.extensionName << "\n";
    }

    uint32_t glfwExtensionCount;
    std::vector<const char*> requiredExtensions;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (int i = 0; i < glfwExtensionCount; ++i) {
      requiredExtensions.emplace_back(glfwExtensions[i]);
    }

#if defined __APPLE__ && defined __arm64__
    requiredExtensions.emplace_back(
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    requiredExtensions.emplace_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    std::cout << "\nrequired extensions:\n";
    for (const auto& extensionName : requiredExtensions) {
      std::cout << extensionName << " ";
      if (extensions.end() !=
          std::find_if(extensions.begin(), extensions.end(),
                       [extensionName](const auto& ext) {
                         return std::strcmp(extensionName, ext.extensionName) ==
                                0;
                       })) {
        std::cout << "FOUND!\n";
      } else {
        std::cout << "MISSING!\n";
      }
    }

    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (enableValidationLayers) {
      createInfo.enabledLayerCount =
          static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
      throw std::runtime_error("Incompatible driver");
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to create Vulkan instance");
    }
  }

  void pickPhysicalDevice() {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
      throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
      if (isDeviceSuitable(device, surface)) {
        physicalDevice = device;
        msaaSamples = getMaxUsableSampleCount(physicalDevice);
        break;
      }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
      throw std::runtime_error("No suitable GPU found");
    }
  }

  bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const {
    auto indices = findQueueFamilies(device, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
      auto swapChainSupport = querySwapChainSupport(device, surface);
      swapChainAdequate = !(swapChainSupport.formats.empty() ||
                            swapChainSupport.presentModes.empty());
    }

    VkPhysicalDeviceFeatures supportedFeatures{};
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate &&
           supportedFeatures.samplerAnisotropy;
  }

  bool checkDeviceExtensionSupport(VkPhysicalDevice device) const {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         extensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                             deviceExtensions.end());

    std::cout << "\n";
    for (const auto& extension : extensions) {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      std::span<VkSurfaceFormatKHR const> formats) {
    // Prefer 32-bit SRGB, otherwise just return the first format available
    for (const auto& availableFormat : formats) {
      if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
      }
    }

    return formats.front();
  }

  VkPresentModeKHR chooseSwapPresentMode(
      std::span<VkPresentModeKHR const> presentModes) {
    // Prefer "triple-buffering"
    for (const auto& availableMode : presentModes) {
      if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return availableMode;
      }
    }
    // guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
      // current extent is fine. Use it.
      return capabilities.currentExtent;
    } else {
      // Derive extent from window size
      int width, height;
      glfwGetWindowSize(window, &width, &height);

      VkExtent2D actualExtent{static_cast<uint32_t>(width),
                              static_cast<uint32_t>(height)};

      actualExtent.width =
          std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                     capabilities.maxImageExtent.width);
      actualExtent.height =
          std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                     capabilities.maxImageExtent.height);

      return actualExtent;
    }
  }

  static VkSampleCountFlagBits getMaxUsableSampleCount(
      VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    // need to check both color and depth sample maximums
    VkSampleCountFlags counts =
        deviceProperties.limits.framebufferColorSampleCounts &
        deviceProperties.limits.framebufferDepthSampleCounts;

    for (int bits = 64; bits >= 1; bits /= 2) {
      if (hasFlags(counts, bits)) {
        return static_cast<VkSampleCountFlagBits>(bits);
      }
    }

    return VK_SAMPLE_COUNT_1_BIT;
  }

  static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                              VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             queueFamilies.data());

    for (int i = 0; i < queueFamilies.size(); ++i) {
      if (hasFlags(queueFamilies[i].queueFlags, VK_QUEUE_GRAPHICS_BIT)) {
        indices.graphicsFamily = i;
      }

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

      if (presentSupport) {
        indices.presentFamily = i;
      }

      if (indices.isComplete()) {
        break;
      }
    }

    return indices;
  }

  static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                                       VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         nullptr);
    if (formatCount > 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                           details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &presentModeCount, nullptr);
    if (presentModeCount > 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
  }

  void createLogicalDevice() {
    auto indices = findQueueFamilies(physicalDevice, surface);

    VkPhysicalDeviceFeatures deviceFeatures{.samplerAnisotropy = VK_TRUE};

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies{indices.graphicsFamily.value(),
                                           indices.presentFamily.value()};
    float queuePriority = 1.f;
    for (auto queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    deviceCreateInfo.enabledExtensionCount =
        static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) !=
        VK_SUCCESS) {
      throw std::runtime_error("Unable to create logical device");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
  }

  void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create window surface");
    }
  }

  void createBuffer(VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer& buffer,
                    VkDeviceMemory& memory) const {
    VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                  .size = size,
                                  .usage = usage,
                                  .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to create vertex buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits, properties)};

    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate vertex buffer memory");
    }

    vkBindBufferMemory(device, buffer, memory, 0);
  }

  void createImage(uint32_t width,
                   uint32_t height,
                   uint32_t mipLevels,
                   VkSampleCountFlagBits numSamples,
                   VkFormat format,
                   VkImageTiling tiling,
                   VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   VkImage& image,
                   VkDeviceMemory& memory) {
    VkImageCreateInfo imageInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                .imageType = VK_IMAGE_TYPE_2D,
                                .extent.width = static_cast<uint32_t>(width),
                                .extent.height = static_cast<uint32_t>(height),
                                .extent.depth = 1,
                                .mipLevels = mipLevels,
                                .arrayLayers = 1,
                                .format = format,
                                .tiling = tiling,
                                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                .usage = usage,
                                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                .samples = numSamples};

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits, properties)};

    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate image memory");
    }

    vkBindImageMemory(device, image, memory, 0);
  }

  void initVulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();

    createDescriptorSetLayout();

    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();

    // TODO: DRY - duplicated in recreateSwapChain
    createColorResources();
    createDepthResources();
    createFramebuffers();

    loadModel();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();

    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();

    createDescriptorPool();
    createDescriptorSets();

    createSyncObjects();
  }

  void createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{
        {{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
         {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)}}};

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
        .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)};

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor pool");
    }
  }

  void createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to allocate descriptor sets");
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      VkDescriptorBufferInfo bufferInfo{.buffer = uniformBuffers[i],
                                        .offset = 0,
                                        .range = sizeof(UniformBufferObject)};

      VkDescriptorImageInfo imageInfo{
          .imageView = textureImageView,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .sampler = textureSampler};

      std::array<VkWriteDescriptorSet, 2> descriptorWrites{
          {{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,  // descriptors can be arrays
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &bufferInfo},
           {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo = &imageInfo}}};

      vkUpdateDescriptorSets(device, descriptorWrites.size(),
                             descriptorWrites.data(), 0, nullptr);
    }
  }

  void createDescriptorSetLayout() {
    // Uniforms for vertex transforms
    VkDescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT};

    // combined sampler for texture-mapping in the fragment shader
    VkDescriptorSetLayoutBinding samplerLayoutBinding{
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImmutableSamplers = nullptr,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT};

    std::array bindings{uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = bindings.size(),
        .pBindings = bindings.data()};

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor set layout");
    }
  }

  void createBufferAndTransferData(std::ranges::contiguous_range auto&& srcData,
                                   VkBuffer& dstBuffer,
                                   VkDeviceMemory& dstMemory,
                                   VkBufferUsageFlags usage) const {
    VkDeviceSize bufferSize = sizeof(srcData.back()) * srcData.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    createHostVisibleBuffer(srcData, stagingBuffer, stagingMemory,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    createBuffer(bufferSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dstBuffer, dstMemory);

    copyBuffer(stagingBuffer, dstBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
  }

  void createHostVisibleBuffer(std::ranges::contiguous_range auto&& srcData,
                               VkBuffer& dstBuffer,
                               VkDeviceMemory& dstMemory,
                               VkBufferUsageFlags usage) const {
    VkDeviceSize bufferSize = sizeof(srcData.back()) * srcData.size();

    createBuffer(bufferSize, usage,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 dstBuffer, dstMemory);

    void* data;  // host (CPU) memory that is mapped to the buffer
    vkMapMemory(device, dstMemory, 0, bufferSize, 0, &data);
    // copy the data
    memcpy(data, srcData.data(), bufferSize);
    vkUnmapMemory(device, dstMemory);
  }

  void createIndexBuffer() {
    createBufferAndTransferData(indices, indexBuffer, indexBufferMemory,
                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  }

  void createVertexBuffer() {
    createBufferAndTransferData(vertices, vertexBuffer, vertexBufferMemory,
                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  }

  void createUniformBuffers() {
    const VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   uniformBuffers[i], uniformBuffersMemory[i]);
      vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0,
                  &uniformBuffersMapped[i]);
    }
  }

  ScopedCommandBuffer createCommandScope() const {
    return {device, commandPool, graphicsQueue};
  }

  void loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err, warn;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          MODEL_PATH)) {
      throw std::runtime_error("failed to load model");
    }

    // merge duplicate vertices
    std::unordered_map<Vertex, uint32_t> uniqueVerts;
    for (const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
        Vertex vert{.pos{
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2],
                    },
                    // obj format has 0 at the bottom of the image, but our
                    // image was loaded top-to-bottom
                    .texCoord{
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.f - attrib.texcoords[2 * index.texcoord_index + 1],
                    },
                    .color{1.f, 1.f, 1.f}};
        if (const auto iter = uniqueVerts.find(vert);
            iter != uniqueVerts.end()) {
          indices.push_back(iter->second);
        } else {
          auto index = vertices.size();
          uniqueVerts.emplace(vert, index);
          vertices.push_back(vert);
          indices.push_back(index);
        }
      }
    }
  }

  void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH, &texWidth, &texHeight,
                                &texChannels, STBI_rgb_alpha);

    if (!pixels) {
      throw std::runtime_error("failed to load texture image!");
    }

    mipLevels = computeMipLevels(texWidth, texHeight);

    VkDeviceSize imageSize = texWidth * texHeight * texChannels * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createHostVisibleBuffer(std::span(pixels, imageSize), stagingBuffer,
                            stagingBufferMemory,
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

    stbi_image_free(pixels);

    // TODO: figure how why the texture images are loaded as linear instead of
    // SRGB (maybe something in the STB library?)
    const VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    // set the transfer_dst and transfer_src flags becase we need to read it
    // to create mipmaps
    createImage(
        texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, imageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    // transition the image for mipmap creation
    transitionImageLayout(textureImage, imageFormat, mipLevels,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyBufferToImage(stagingBuffer, textureImage,
                      static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));

    // This leaves the image in the VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    // layout
    generateMipmaps(textureImage, imageFormat, texWidth, texHeight, mipLevels);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
  }

  void createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                       mipLevels, VK_IMAGE_ASPECT_COLOR_BIT);
  }

  void generateMipmaps(VkImage image,
                       VkFormat imageFormat,
                       uint32_t width,
                       uint32_t height,
                       uint32_t mipLevels) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat,
                                        &formatProperties);

    if (!hasFlags(formatProperties.optimalTilingFeatures,
                  VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
      throw std::runtime_error(
          "failed to generate mipmaps: linear filtering not supported");
    }
    auto commandBuffer = createCommandScope();
    // need a barrier to transition each level individually
    // (our transitionImage function only does the entire image)
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = image,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseArrayLayer = 0,
            .layerCount = 1,
            .levelCount = 1,
        }};

    int32_t mipWidth = width, mipHeight = height;
    for (uint32_t i = 1; i < mipLevels; i++) {
      // Make readable the base level (the one above the level we're creating)
      // barrier will wait on the base level to be filled, before we proceed
      // with blitting the next level
      barrier.subresourceRange.baseMipLevel = i - 1;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                           nullptr, 1, &barrier);

      int32_t dstWidth = std::max(mipWidth / 2l, 1l);
      int32_t dstHeight = std::max(mipHeight / 2l, 1l);
      // for 2d images, the Z dimension has a size of 1
      VkImageBlit blit{.srcOffsets{{0, 0, 0}, {mipWidth, mipHeight, 1}},
                       .srcSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                       .baseArrayLayer = 0,
                                       .layerCount = 1,
                                       .mipLevel = i - 1},
                       .dstOffsets{{0, 0, 0}, {dstWidth, dstHeight, 1}},
                       .dstSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                       .baseArrayLayer = 0,
                                       .layerCount = 1,
                                       .mipLevel = i}};

      // We're blitting between different mip levels of the same VkImage,
      // doing a linear downscaling
      vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                     VK_FILTER_LINEAR);

      // Make the source mip image usable by the shader
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                           0, nullptr, 1, &barrier);

      mipWidth = dstWidth;
      mipHeight = dstHeight;
    }

    // Barrier for the last mip level that wasn't handled in the loop
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);
  }

  VkImageView createImageView(VkImage image,
                              VkFormat format,
                              uint32_t mipLevels,
                              VkImageAspectFlags aspectFlags) const {
    VkImageViewCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange{.aspectMask = aspectFlags,
                          .baseMipLevel = 0,
                          .levelCount = mipLevels,
                          .baseArrayLayer = 0,
                          .layerCount = 1}};

    VkImageView imageView;
    if (vkCreateImageView(device, &createInfo, nullptr, &imageView) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create image view");
    }

    return imageView;
  }

  void createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples,
                colorFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage,
                colorImageMemory);

    colorImageView =
        createImageView(colorImage, colorFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
  }

  void createDepthResources() {
    auto depthFormat = findDepthFormat();
    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples,
                depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage,
                depthImageMemory);
    depthImageView =
        createImageView(depthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

    transitionImageLayout(depthImage, depthFormat, 1, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  }

  VkFormat findSupportedCandidates(std::span<VkFormat const> candidates,
                                   VkImageTiling tiling,
                                   VkFormatFeatureFlags features) const {
    for (auto format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
      if (tiling == VK_IMAGE_TILING_LINEAR &&
          hasFlags(props.linearTilingFeatures, features)) {
        return format;
      } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                 hasFlags(props.optimalTilingFeatures, features)) {
        return format;
      }
    }

    throw std::runtime_error("failed to find supported format");
  }

  VkFormat findDepthFormat() const {
    return findSupportedCandidates(
        std::array{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                   VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  static bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
  }

  void createTextureSampler() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    VkSamplerCreateInfo samplerInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.f,
        .minLod = 0.f,
        .maxLod = static_cast<float>(mipLevels)};

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create texture sampler");
    }
  }

  void copyBuffer(VkBuffer srcBuffer,
                  VkBuffer dstBuffer,
                  VkDeviceSize size) const {
    auto commandBuffer = createCommandScope();
    VkBufferCopy copyRegion{.size = size};
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  }

  void transitionImageLayout(VkImage image,
                             VkFormat format,
                             uint32_t mipLevels,
                             VkImageLayout oldLayout,
                             VkImageLayout newLayout) {
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                          .baseMipLevel = 0,
                          .levelCount = mipLevels,
                          .baseArrayLayer = 0,
                          .layerCount = 1}};

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      if (hasStencilComponent(format)) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
    }

    VkPipelineStageFlags srcStage, dstStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;  // don't need to wait on anything
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      // since we're not waiting on anything, the source stage should be set to
      // the earliest possible stage in the pipeline
      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      // reads in early_fragment_tests, writes in late_fragment_tests
      // pick the earliest stage that needs the depth buffer
      dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
      throw std::runtime_error("invalid image layout transition");
    }

    // Common way to perform a layout transition is employing a memory barrier
    auto commandBuffer = createCommandScope();
    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage,
                         0,             // dependency flags
                         0, nullptr,    // memory barriers
                         0, nullptr,    // buffer memory barriers
                         1, &barrier);  // image memory barriers
  }

  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if (hasBit(typeFilter, i) &&
          hasFlags(memProperties.memoryTypes[i].propertyFlags, properties)) {
        // found it
        return i;
      }
    }

    throw std::runtime_error("unable to find suitable memory type");
  }

  void copyBufferToImage(VkBuffer buffer,
                         VkImage image,
                         uint32_t width,
                         uint32_t height) {
    auto commandBuffer = createCommandScope();
    VkBufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,    // indicates tightly packed
        .bufferImageHeight = 0,  // indicates tightly packed
        .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                          .mipLevel = 0,
                          .baseArrayLayer = 0,
                          .layerCount = 1},
        .imageOffset{.x = 0, .y = 0, .z = 0},
        .imageExtent{.width = width, .height = height, .depth = 1}};

    vkCmdCopyBufferToImage(
        commandBuffer, buffer, image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // current layout of the dest
                                               // image
        1, &region);
  }

  void createCommandPool() {
    auto queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool");
    }
  }

  void createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = commandBuffers.size();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers");
    }
  }

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    // Because we specified LOAD_OP_CLEAR, we need to specific a clear colour
    // value.
    // clear values defined in the order of attachment indices (layout)
    std::array<VkClearValue, 2> clearValues{{
        {.color{0.f, 0.f, 0.f, 1.0}},
        {.depthStencil{1.f, 0}}  // 1.0 is the furthest possible depth
    }};
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);

    VkBuffer vertexBuffers[]{vertexBuffer};
    VkDeviceSize offsets[]{0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSets[currentFrame],
                            0, nullptr);

    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Set up viewport and scissor dynamically
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // DRAW IT!
    vkCmdDrawIndexed(commandBuffer, indices.size(), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer");
    }
  }

  void createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainFramebuffers.size(); ++i) {
      std::array attachments{colorImageView, depthImageView,
                             swapChainImageViews[i]};

      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.attachmentCount = attachments.size();
      framebufferInfo.pAttachments = attachments.data();
      framebufferInfo.renderPass = renderPass;
      framebufferInfo.width = swapChainExtent.width;
      framebufferInfo.height = swapChainExtent.height;
      framebufferInfo.layers = 1;  // single images

      if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                              &swapChainFramebuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer");
      }
    }
  }

  void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                            &imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                            &renderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
              VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphores");
      }
    }
  }

  void createRenderPass() {
    // Attachment 0: color
    VkAttachmentDescription colorAttachment{
        .format = swapChainImageFormat,
        .samples = msaaSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentReference colorAttachmentRef{
        .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    // Attachment 1: depth
    VkAttachmentDescription depthAttachment{
        .format = findDepthFormat(),
        .samples = msaaSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        // contents not needed after rendering
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkAttachmentReference depthAttachmentRef{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    // Attachment 2: Presentation (swap chain image)
    // multisampled color attachments must be resolved to a "regular" image
    // for presentation
    VkAttachmentDescription colorResolveAttachment{
        .format = swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    VkAttachmentReference colorResolveAttachmentRef{
        .attachment = 2, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
        .pResolveAttachments = &colorResolveAttachmentRef};

    std::array attachments{colorAttachment, depthAttachment,
                           colorResolveAttachment};
    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    // This part is a bit confusing and I'm not sure if I understand it
    // completely.
    // We're using Vulkan's automatic layout transition, which happens if it
    // has an initial layout of VK_IMAGE_LAYOUT_UNDEFINED. There is an implicit
    // subpass (SUBPASS_EXTERNAL) that handles the image layout transition.
    // For correctness, we need to wait for the transition before we can write
    // to the image attachment. We do not need to do this if we instead handle
    // the layout transitions explicitly ourselves.
    VkSubpassDependency dependency{};
    // implicit subpass before our subpass
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;  // index of our subpass

    // Source pass: implicit transition
    // stage(s) to wait on
    // The confusing bit is that these are not necessarily the the stages of
    // the external subpass that we're waiting on. It's set this way so that
    // we form a dependency chain that is [acquisition]->[layout]->[render],
    // were [acquisition] signals a semaphore set to the COLOR_ATTACHMENT_OUTPUT
    // stage. At least this is my understanding. This is poorly-documented in
    // the Vulkan literature.
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    // We're not accessing the memory involved in this subpass
    dependency.srcAccessMask = 0;

    // Wait for the src pass to finish before executing the
    // STAGE_COLOR_ATTACHMENT_OUTPUT and
    // STAGE_EARLY_FRAGMENT_TESTS stages
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    // indicates that our subpass needs to be able to write to the attachments
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create render pass");
    }
  }

  void createGraphicsPipeline() {
    auto vertShaderModule = createShaderModule(readFile(VERT_SHADER_SPV));
    auto fragShaderModule = createShaderModule(readFile(FRAG_SHADER_SPV));

    VkPipelineShaderStageCreateInfo vertShaderStage{};
    vertShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStage.module = vertShaderModule;
    vertShaderStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStage{};
    fragShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStage.module = fragShaderModule;
    fragShaderStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[]{vertShaderStage,
                                                   fragShaderStage};

    std::array dynamicStates{VK_DYNAMIC_STATE_VIEWPORT,
                             VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()};

    VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
    assemblyInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkRect2D scissor;
    scissor.extent = swapChainExtent;
    scissor.offset = {0, 0};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    // actually viewport and scissor will be set up later (dynamic)
    viewportState.scissorCount = 1;
    viewportState.viewportCount = 1;
    // viewportState.pViewports = &viewport;
    // viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    // because of our projection matrix flip, the windings are reversed
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.f;

    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = msaaSamples};

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout};

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout");
    }

    // enable depth testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        // fragment should be draw if its depth is LESS
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE};

    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,

        .pDynamicState = &dynamicState,
        .pInputAssemblyState = &assemblyInfo,
        .pVertexInputState = &vertexInputInfo,
        .pRasterizationState = &rasterizer,
        .pViewportState = &viewportState,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDepthStencilState = &depthStencil,

        .layout = pipelineLayout,
        .renderPass = renderPass,
        .subpass = 0};

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline");
    }

    // Once the pipeline has been created, we don't need the shader modules
    // anymore
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
  }

  VkShaderModule createShaderModule(std::span<const char> code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (VK_SUCCESS !=
        vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule)) {
      throw std::runtime_error("failed to create shader module");
    }

    return shaderModule;
  }

  void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (int i = 0; i < swapChainImageViews.size(); ++i) {
      swapChainImageViews[i] =
          createImageView(swapChainImages[i], swapChainImageFormat, 1,
                          VK_IMAGE_ASPECT_COLOR_BIT);
    }
  }

  void createSwapChain() {
    auto swapChainSupport = querySwapChainSupport(physicalDevice, surface);
    auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    auto extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices = findQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[]{indices.graphicsFamily.value(),
                                  indices.presentFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily) {
      // Use concurrent mode to use multiple families in this swap chain. This
      // saves us from having to do ownership transfers, but is less performant.
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
      // graphics and present are the same queue
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                            swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
  }

  void cleanupSwapChain() {
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
      vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (auto imageView : swapChainImageViews) {
      vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);
  }

  void recreateSwapChain() {
    std::cout << "recreating swap chain" << std::endl;
    int width = 0, height = 0;
    glfwGetWindowSize(window, &width, &height);
    // loop if the window is minimized
    while (width == 0 || height == 0) {
      glfwWaitEvents();  // sleep until something happens
      glfwGetWindowSize(window, &width, &height);
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createFramebuffers();
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      drawFrame();
    }

    vkDeviceWaitIdle(device);
  }

  void drawFrame() {
    // This fence ensures that we can start re-using the command buffer
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                    std::numeric_limits<uint64_t>::max());

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      // Not possible to present to the swap chain in this state
      recreateSwapChain();
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image");
    }

    // only reset the fence if we are submitting work
    // if we need to recreate the swap chain, no rendering is performed and
    // inFlightFences[i] never gets signalled.
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    updateUniformBuffer(currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Wait for imageAvailableSemaphore at the
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage of the pipeline since
    // we want an image to be available before writing colors to it.
    VkSemaphore waitSemaphores[]{imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[]{renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                      inFlightFences[currentFrame]) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[]{swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    // Present the results!!
    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        framebufferResized) {
      framebufferResized = false;
      recreateSwapChain();
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    UniformBufferObject ubo{};
    // rotate the model around the z-axis at 90 degrees/s
    ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f),
                            glm::vec3(0, 0, 1.f));
    ubo.view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0, 0, 0),
                           glm::vec3(0, 0, 1.f));  // up is +z
    ubo.proj = glm::perspective(
        glm::radians(45.f),
        swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.f);

    // switch from OpenGL convention for clip coordinates (y up) to Vulkan
    // convention (y down) invert the y scaling factor in the projection matrix
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
  }

  void cleanup() {
    cleanupSwapChain();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroyBuffer(device, uniformBuffers[i], nullptr);
      vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyImageView(device, textureImageView, nullptr);
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    vkDestroySampler(device, textureSampler, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
      vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
      vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    // This also frees all command buffers (which are owned by the pool)
    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};

int main() {
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}