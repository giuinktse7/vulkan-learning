

#include "engine.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "validation.h"
#include "vulkan_helpers.h"
#include "vulkan_debug.h"
#include "../logger.h"
#include "../file.h"
#include "vertex.h"

#include <imgui_impl_glfw.h>

Engine *g_engine;

namespace engine
{
  namespace
  {
    std::unique_ptr<Engine> g_engine_container = nullptr;
  }

  void create()
  {
    if (g_engine)
    {
      ABORT_PROGRAM("The engine is already created.");
    }

    g_engine_container = std::make_unique<Engine>();
    g_engine = g_engine_container.get();
  }

} // namespace engine

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

VkResult Engine::mapMemory(
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void **ppData)
{
  return vkMapMemory(device, memory, offset, size, flags, ppData);
}

Engine::Engine()
    : frame(&frames[0])
{
}

Engine::~Engine()
{
}

void Engine::initialize(GLFWwindow *window)
{
  if (isInitialized)
  {
    throw std::runtime_error("Engine is already initialized");
  }

  this->window = window;

  createVulkanInstance();

  VulkanDebug::setupDebugMessenger(instance, debugMessenger);

  createSurface();

  this->physicalDevice = DeviceManager::pickPhysicalDevice();
  this->device = DeviceManager::createLogicalDevice();

  this->queueFamilyIndices = DeviceManager::getQueueFamilies(this->physicalDevice);

  swapChain.initialize();
  createCommandPool();
  mapRenderer->initialize();
  gui.initialize();

  createSyncObjects();

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  mapRenderer->camera.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));

  mapRenderer->camera.updateZoom();

  isInitialized = true;
}

void Engine::createVulkanInstance()
{
  if (Validation::enableValidationLayers && !checkValidationLayerSupport())
  {
    throw std::runtime_error("Validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Renderer";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Renderer Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (Validation::enableValidationLayers)
  {
    createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanHelpers::validationLayers.size());
    createInfo.ppEnabledLayerNames = VulkanHelpers::validationLayers.data();

    VulkanDebug::populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  }
  else
  {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, allocator, &instance) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create instance!");
  }
}

void Engine::createSurface()
{
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create windows surface!");
  }
}

void Engine::framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
  auto app = reinterpret_cast<Engine *>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

bool Engine::checkValidationLayerSupport()
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  if (!chronosOrStandardValidation(availableLayers))
  {
    return false;
  }

  for (const char *layerName : VulkanHelpers::validationLayers)
  {
    bool layerFound = std::any_of(
        availableLayers.begin(),
        availableLayers.end(),
        [layerName](VkLayerProperties k) { return strcmp(layerName, k.layerName) == 0; });

    if (!layerFound)
    {
      return false;
    }
  }

  return true;
}

std::vector<const char *> Engine::getRequiredExtensions()
{
  uint32_t glfwExtensionCount = 0;

  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (Validation::enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

bool Engine::chronosOrStandardValidation(std::vector<VkLayerProperties> &props)
{
  return std::any_of(
      props.begin(),
      props.end(),
      [](VkLayerProperties k) {
        return (strcmp(VulkanHelpers::khronosValidation, k.layerName) == 0) || strcmp(VulkanHelpers::standardValidation, k.layerName) == 0;
      });
}

uint32_t Engine::getMaxFramesInFlight()
{
  return static_cast<uint32_t>(swapChain.getImages().size());
}

VkCommandBuffer Engine::beginSingleTimeCommands()
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void Engine::endSingleTimeCommands(VkCommandBuffer buffer)
{
  vkEndCommandBuffer(buffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &buffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &buffer);
}

void Engine::createCommandPool()
{
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  poolInfo.flags = 0; // Optional

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create command pool!");
  }
}

void Engine::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;

  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(
      commandBuffer,
      sourceStage, destinationStage,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void Engine::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(
      commandBuffer,
      buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  endSingleTimeCommands(commandBuffer);
}

bool Engine::initFrame()
{
  if (!swapChain.isValid())
  {
    if (!isValidWindowSize())
    {
      return false;
    }

    recreateSwapChain();
  }

  vkWaitForFences(device, 1, &frame->inFlightFence, VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;

  VkResult result = vkAcquireNextImageKHR(
      device,
      swapChain.get(),
      UINT64_MAX,
      frame->imageAvailableSemaphore,
      VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
  {
    recreateSwapChain();
  }
  if (framebufferResized)
  {
    framebufferResized = false;
    recreateSwapChain();
    return initFrame();
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  if (imageFences[imageIndex] != VK_NULL_HANDLE)
  {
    vkWaitForFences(device, 1, &imageFences[imageIndex], VK_TRUE, UINT64_MAX);
  }

  imageFences[imageIndex] = frame->inFlightFence;

  vkResetFences(device, 1, &frame->inFlightFence);

  return true;
}

void Engine::nextFrame()
{
  if (initFrame())
  {
    mapRenderer->recordFrame(currentFrameIndex);
    gui.recordFrame(currentFrameIndex);

    endFrame();
  }
}

void Engine::createSyncObjects()
{
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (auto &frame : frames)
  {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.renderCompleteSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &frame.inFlightFence) != VK_SUCCESS)
    {

      throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
  }
}

VkShaderModule Engine::createShaderModule(const std::vector<uint8_t> &code)
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

std::shared_ptr<Texture> Engine::CreateTexture(const std::string &filename)
{
  return std::make_shared<Texture>(filename);
}

void Engine::setFrameIndex(uint32_t index)
{
  prevFrame = frame;
  frame = &frames[index];
  currentFrameIndex = index;
}

void Engine::recreateSwapChain()
{
  swapChain.recreate();
  imageFences.fill(VK_NULL_HANDLE);

  mapRenderer->recreate();
  gui.recreate();

  setFrameIndex(0);
}

void Engine::endFrame()
{
  mapRenderer->endFrame();
  gui.endFrame(currentFrameIndex);

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  std::array<VkCommandBuffer, 2> submitCommandBuffers = {
      mapRenderer->getCommandBuffer(),
      gui.getCommandBuffer(currentFrameIndex)};

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &frame->imageAvailableSemaphore;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
  submitInfo.pCommandBuffers = submitCommandBuffers.data();
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &frame->renderCompleteSemaphore;

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame->inFlightFence) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit command buffer to the graphics queue");
  }

  presentFrame();

  setFrameIndex((currentFrameIndex + 1) % getMaxFramesInFlight());
}

void Engine::presentFrame()
{
  VkSwapchainKHR swapChains[] = {swapChain.get()};
  uint32_t imageIndices[] = {currentFrameIndex};

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &frame->renderCompleteSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = imageIndices;

  auto result = vkQueuePresentKHR(*getPresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
  {
    Logger::info("recreate in presentFrame");
    framebufferResized = false;
    recreateSwapChain();
  }
  else if (result != VK_SUCCESS)
  {
    throw std::runtime_error("failed to present swap chain image");
  }
}

void Engine::WaitUntilDeviceIdle()
{
  vkDeviceWaitIdle(device);
}

void Engine::cleanupSyncObjects()
{
  for (auto &frame : frames)
  {
    vkDestroySemaphore(device, frame.imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, frame.renderCompleteSemaphore, nullptr);
    vkDestroyFence(device, frame.inFlightFence, nullptr);
  }
}

bool Engine::isValidWindowSize()
{
  int width = 0, height = 0;
  glfwGetFramebufferSize(getWindow(), &width, &height);
  return !(width == 0 || height == 0);
}

void Engine::shutdown()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  vkDestroyDescriptorPool(device, gui.getDescriptorPool(), nullptr);
}