

#include "engine.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "validation.h"
#include "VulkanHelpers.h"
#include "VulkanDebug.h"
#include "Logger.h"
#include "file.h"
#include "vertex.h"
#include "pipeline.h"
#include "buffer.h"

namespace Render
{
  struct UniformBufferObject
  {
    glm::vec2 extent;
  };
} // namespace Render

/* 
  Singleton variables
*/
Engine *Engine::pinstance_{nullptr};
std::mutex Engine::mutex_;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

Engine::Engine()
{
}

Engine::~Engine()
{
}

/**
 * The first time we call GetInstance we will lock the storage location
 *      and then we make sure again that the variable is null and then we
 *      set the value. RU:
 */
Engine *Engine::GetInstance()
{
  if (pinstance_ == nullptr)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pinstance_ == nullptr)
    {
      pinstance_ = new Engine();
    }
  }
  return pinstance_;
}

void Engine::setInstance(Engine *instance)
{
  pinstance_ = instance;
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
  DeviceManager::pickPhysicalDevice();
  DeviceManager::createLogicalDevice();

  swapChain = SwapChain();
  swapChain.init();

  renderPass = mapRenderer->createRenderPass();
  createDescriptorSetLayouts();
  createGraphicsPipeline();
  swapChain.createFramebuffers();
  createCommandPool();
  createUniformBuffers();
  createDescriptorPool();
  createPerFrameDescriptorSets();
  createSyncObjects();
  createIndexAndVertexBuffers();

  uint8_t whitePixel[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  defaultTexture = std::make_shared<Texture>(1, 1, &whitePixel[0]);

  isInitialized = true;
}

void Engine::init()
{
  Logger::info("Initializing engine..");
  initWindow();
  createVulkanInstance();

  VulkanDebug::setupDebugMessenger(instance, debugMessenger);

  createSurface();
  DeviceManager::pickPhysicalDevice();
  DeviceManager::createLogicalDevice();

  swapChain = SwapChain();

  swapChain.init();
}

void Engine::initWindow()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  setWindow(glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr));
  glfwSetWindowUserPointer(window, this);

  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
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

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
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
  Logger::info("Changed");
  auto app = reinterpret_cast<Engine *>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

bool Engine::checkValidationLayerSupport()
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  // for (const auto &layerProp : availableLayers)
  // {
  // 	std::cout << layerProp.layerName << std::endl;
  // }

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

void Engine::createCommandPool()
{
  QueueFamilyIndices queueFamilyIndices = VulkanHelpers::findQueueFamilies(physicalDevice, surface);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  poolInfo.flags = 0; // Optional

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create command pool!");
  }

  auto framesInFlight = getMaxFramesInFlight();
  perFrameCommandPool.resize(framesInFlight);
  perFrameCommandBuffer.resize(framesInFlight);
  for (int i = 0; i < framesInFlight; ++i)
  {
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &perFrameCommandPool[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create graphics command pool!");
    }
  }
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

  vkQueueSubmit(*getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(*getGraphicsQueue());

  vkFreeCommandBuffers(device, commandPool, 1, &buffer);
}

void Engine::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
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
  barrier.subresourceRange.levelCount = mipLevels;
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
  region.imageExtent = {
      width,
      height,
      1};

  vkCmdCopyBufferToImage(
      commandBuffer,
      buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  endSingleTimeCommands(commandBuffer);
}

void Engine::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
  // Check if image format supports linear blitting
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
  {
    throw std::runtime_error("texture image format does not support linear blitting!");
  }

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++)
  {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    VkImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(commandBuffer,
                   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void Engine::recordCommands()
{
  allocateCommandBuffers();
  for (uint32_t i = 0; i < commandBuffers.size(); ++i)
  {
    beginRenderPass();

    renderSprites(i);

    endRenderPass();
  }
}

void Engine::allocateCommandBuffers()
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  if (vkAllocateCommandBuffers(getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void Engine::beginRenderPass()
{
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChain.getFrameBuffer(nextFrame);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChain.getExtent();
  renderPassInfo.clearValueCount = 1;
  VkClearValue clearValue = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
  renderPassInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(currentCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Engine::endRenderPass() const
{
  vkCmdEndRenderPass(currentCommandBuffer);
}

void Engine::renderSprites(size_t bufferIndex)
{

  for (auto sprite : sprites)
  {
    vkCmdBindPipeline(commandBuffers[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, sprite.pipeline.vulkanPipeline);

    renderSprite(bufferIndex, sprite);
  }
}

void Engine::createCommandBuffers()
{

  // VkCommandBufferAllocateInfo allocInfo{};
  // allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  // allocInfo.commandPool = commandPool;
  // allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  // allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  // if (vkAllocateCommandBuffers(getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
  // {
  //   throw std::runtime_error("failed to allocate command buffers!");
  // }

  // for (size_t i = 0; i < commandBuffers.size(); ++i)
  // {
  //   VkCommandBufferBeginInfo beginInfo{};
  //   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  //   if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("failed to begin recording command buffer!");
  //   }

  //   VkRenderPassBeginInfo renderPassInfo{};
  //   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  //   renderPassInfo.renderPass = renderPass;
  //   renderPassInfo.framebuffer = swapChain.getFrameBuffer(i);

  //   renderPassInfo.renderArea.offset = {0, 0};
  //   renderPassInfo.renderArea.extent = swapChain.getExtent();

  //   VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
  //   renderPassInfo.clearValueCount = 1;
  //   renderPassInfo.pClearValues = &clearColor;

  //   vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  //   vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  //   for (auto sprite : sprites)
  //   {
  //     renderSprite(i, sprite);
  //   }

  //   vkCmdEndRenderPass(commandBuffers[i]);

  //   if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("failed to record command buffer!");
  //   }
  // }
}

void Engine::beginRenderCommands()
{
}
void Engine::endRenderCommands()
{
}

void Engine::renderSprite(size_t bufferIndex, Sprite &sprite)
{
  VkBuffer vertexBuffers[] = {sprite.getVertexBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffers[bufferIndex], 0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffers[bufferIndex], sprite.getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(commandBuffers[bufferIndex],
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          sprite.pipeline.layout,
                          0,
                          1,
                          &sprite.getDescriptorSet(bufferIndex),
                          0,
                          nullptr);

  vkCmdDrawIndexed(commandBuffers[bufferIndex], static_cast<uint32_t>(sprite.getIndices().size()), 1, 0, 0, 0);
}

void Engine::createSyncObjects()
{
  imageAvailableSemaphores.resize(getMaxFramesInFlight());
  renderFinishedSemaphores.resize(getMaxFramesInFlight());
  inFlightFences.resize(getMaxFramesInFlight());

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < getMaxFramesInFlight(); i++)
  {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
    {

      throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
  }
}

void Engine::createDescriptorSetLayouts()
{
  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;

  VkDescriptorSetLayoutBinding pf = {};
  pf.binding = 0;
  pf.descriptorCount = 1;
  pf.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pf.pImmutableSamplers = nullptr;
  pf.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  layoutInfo.pBindings = &pf;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &perFrameDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout");
  }

  VkDescriptorSetLayoutBinding po = {};
  po.binding = 0;
  po.descriptorCount = 1;
  po.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  po.pImmutableSamplers = nullptr;
  po.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  layoutInfo.pBindings = &po;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &perTextureDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout");
  }
}

void Engine::createGraphicsPipeline()
{
  std::vector<char> vertShaderCode = File::read("shaders/vert.spv");
  std::vector<char> fragShaderCode = File::read("shaders/frag.spv");

  VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChain.getExtent().width;
  viewport.height = (float)swapChain.getExtent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapChain.getExtent();

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 1.0f;
  colorBlending.blendConstants[1] = 1.0f;
  colorBlending.blendConstants[2] = 1.0f;
  colorBlending.blendConstants[3] = 1.0f;

  std::array<VkDescriptorSetLayout, 2> layouts = {perFrameDescriptorSetLayout, perTextureDescriptorSetLayout};
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = layouts.size();
  pipelineLayoutInfo.pSetLayouts = layouts.data();

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkShaderModule Engine::createShaderModule(VkDevice device, const std::vector<char> &code)
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

void Engine::createUniformBuffers()
{
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers.resize(getMaxFramesInFlight());

  for (size_t i = 0; i < getMaxFramesInFlight(); i++)
  {
    uniformBuffers[i] = (Buffer::create(bufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
  }
}

void Engine::createPerFrameDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> layouts(getMaxFramesInFlight(), perFrameDescriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = getMaxFramesInFlight();
  allocInfo.pSetLayouts = layouts.data();

  perFrameDescriptorSets.resize(getMaxFramesInFlight());
  if (vkAllocateDescriptorSets(device, &allocInfo, &perFrameDescriptorSets[0]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets");
  }

  for (size_t i = 0; i < getMaxFramesInFlight(); ++i)
  {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffers[i].buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrites = {};
    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = perFrameDescriptorSets[i];
    descriptorWrites.dstBinding = 0;
    descriptorWrites.dstArrayElement = 0;
    descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrites, 0, nullptr);
  }
}

void Engine::createIndexAndVertexBuffers()
{
  indexBuffers.resize(getMaxFramesInFlight());
  indexStagingBuffers.resize(getMaxFramesInFlight());
  vertexBuffers.resize(getMaxFramesInFlight());
  vertexStagingBuffers.resize(getMaxFramesInFlight());
}

std::shared_ptr<Texture> Engine::CreateTexture(const std::string &filename)
{
  return std::make_shared<Texture>(filename);
}

bool Engine::StartFrame()
{
  vkWaitForFences(
      device,
      1,
      &inFlightFences[currentFrame],
      VK_TRUE,
      std::numeric_limits<uint64_t>::max());

  VkResult result = vkAcquireNextImageKHR(
      device,
      swapChain.get(),
      std::numeric_limits<uint64_t>::max(),
      imageAvailableSemaphores[currentFrame],
      VK_NULL_HANDLE,
      &nextFrame);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
  {
    Logger::info("StartFrame::recreate");

    swapChain.recreate();
    return false;
  }
  if (framebufferResized)
  {
    framebufferResized = false;
    swapChain.recreate();
    return StartFrame();
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  auto &perFrameCmds = perFrameCommandBuffer[currentFrame];
  if (!perFrameCmds.empty())
  {
    vkFreeCommandBuffers(
        device,
        perFrameCommandPool[currentFrame],
        static_cast<uint32_t>(perFrameCmds.size()),
        &perFrameCmds[0]);
    perFrameCmds.clear();
  }

  currentDescriptorSet = defaultTexture->getDescriptorSet();
  currentTextureWindow = defaultTexture->getTextureWindow();
  currentColor = {1.0f, 1.0f, 1.0f, 1.0f};

  currentBufferIndex = 0;

  startMainCommandBuffer();

  updateUniformBuffer();

  return true;
}

void Engine::EndFrame()
{
  queueCurrentBatch();
  drawBatches();

  if (vkEndCommandBuffer(currentCommandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer");
  }

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &currentCommandBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  if (vkQueueSubmit(*getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit command buffer");
  }

  currentCommandBuffer = nullptr;

  VkSwapchainKHR swapChains[] = {swapChain.get()};
  uint32_t imageIndices[] = {nextFrame};

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = imageIndices;

  auto result = vkQueuePresentKHR(*getPresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
  {
    Logger::info("EndFrame::recreate");
    framebufferResized = false;
    swapChain.recreate();
  }
  else if (result != VK_SUCCESS)
  {
    throw std::runtime_error("failed to present swap chain image");
  }

  currentFrame++;
  currentFrame %= getMaxFramesInFlight();
}

void Engine::startMainCommandBuffer()
{
  auto &perFrameCmds = perFrameCommandBuffer[currentFrame];

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = perFrameCommandPool[currentFrame];
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  perFrameCmds.resize(1);
  if (vkAllocateCommandBuffers(device, &allocInfo, perFrameCmds.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffer");
  }

  currentCommandBuffer = perFrameCmds[0];

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  if (vkBeginCommandBuffer(currentCommandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer");
  }

  vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

void Engine::updateUniformBuffer() const
{
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  Render::UniformBufferObject ubo = {};
  ubo.extent.x = width;
  ubo.extent.y = height;

  void *data;
  vkMapMemory(device, uniformBuffers[currentFrame].bufferMemory, 0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(device, uniformBuffers[currentFrame].bufferMemory);
}

void Engine::SetTexture(std::shared_ptr<Texture> texture)
{
  if (!texture)
  {
    texture = defaultTexture;
  }

  if (texture->getDescriptorSet() != currentDescriptorSet)
  {
    queueDrawCommand();
    currentDescriptorSet = texture->getDescriptorSet();
  }
  currentTextureWindow = texture->getTextureWindow();
}

bool Engine::queueDrawCommand()
{
  if (numIndices > 0)
  {
    drawCommands.emplace_back(currentDescriptorSet, indexOffset, numIndices, currentBufferIndex);
    indexOffset += numIndices;
    numIndices = 0;
    vertexOffset += numVertices;
    numVertices = 0;

    return true;
  }

  return false;
}

void Engine::DrawSprite(float x, float y, float width, float height)
{
  std::array<uint16_t, 6> indices{0, 1, 3, 3, 1, 2};
  std::array<Vertex, 4> vertices{{
      {{x, y}, currentColor, {currentTextureWindow.x0, currentTextureWindow.y0}},
      {{x, y + height}, currentColor, {currentTextureWindow.x0, currentTextureWindow.y1}},
      {{x + width, y + height}, currentColor, {currentTextureWindow.x1, currentTextureWindow.y1}},
      {{x + width, y}, currentColor, {currentTextureWindow.x1, currentTextureWindow.y0}},
  }};

  DrawTriangles(indices.data(), indices.size(), vertices.data(), vertices.size());
}

void Engine::DrawTriangles(const uint16_t *indices, size_t indexCount, const Vertex *vertices, size_t vertexCount)
{
  if (!currentCommandBuffer)
  {
    throw std::runtime_error("no command buffer");
  }

  if (!currentIndexWrite || !currentVertexWrite)
  {
    mapStagingBufferMemory();
  }

  auto vbFull = currentVertexWrite + vertexCount >= vertexWriteEnd;
  auto ibFull = currentIndexWrite + indexCount >= indexWriteEnd;
  if (vbFull || ibFull)
  {
    queueCurrentBatch();
    mapStagingBufferMemory();
  }

  if (!currentIndexWrite || !currentVertexWrite)
  {
    throw std::runtime_error("write destination is null");
  }

  auto base = static_cast<uint16_t>(currentVertexWrite - vertexWriteStart);

  // Both indices and vertices need adjustment while being copied to
  // the index/vertex buffers. The indices need the base added,
  // and the vertices need to be colored, for example.
  auto vertexRead = vertices;
  for (int i = 0; i < vertexCount; ++i)
  {
    currentVertexWrite->pos = vertexRead->pos;
    currentVertexWrite->color = vertexRead->color * currentColor;
    currentVertexWrite->texCoord = vertexRead->texCoord;
    currentVertexWrite->blendMode = currentBlendMode;

    currentVertexWrite++;
    vertexRead++;
  }

  auto indexRead = indices;
  for (int i = 0; i < indexCount; ++i)
  {
    *currentIndexWrite = *indexRead + base;
    currentIndexWrite++;
    indexRead++;
  }

  this->numIndices += indexCount;
  this->numVertices += vertexCount;
}

void Engine::mapStagingBufferMemory()
{
  void *data;

  auto isb = getIndexStagingBuffer();
  vkMapMemory(device, isb.bufferMemory, 0, getIndexBufferSize(), 0, &data);
  indexWriteStart = reinterpret_cast<uint16_t *>(data);
  currentIndexWrite = indexWriteStart;
  indexWriteEnd = indexWriteStart + getMaxNumIndices();

  auto vsb = getVertexStagingBuffer();
  vkMapMemory(device, vsb.bufferMemory, 0, getVertexBufferSize(), 0, &data);
  vertexWriteStart = reinterpret_cast<Vertex *>(data);
  currentVertexWrite = vertexWriteStart;
  vertexWriteEnd = vertexWriteStart + getMaxNumVertices();

  numIndices = 0;
  indexOffset = 0;
  numVertices = 0;
  vertexOffset = 0;
}

BoundBuffer &Engine::getVertexStagingBuffer()
{
  auto &buffers = vertexStagingBuffers[currentFrame];
  if (currentBufferIndex == buffers.size())
  {
    Logger::info("Creating vertex staging buffer");

    auto buffer = Buffer::create(
        getVertexBufferSize(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    buffers.push_back(buffer);
  }

  if (currentBufferIndex > buffers.size())
  {
    throw std::runtime_error("current buffer index value out of range");
  }

  auto &vsb = buffers[currentBufferIndex];
  return vsb;
}

BoundBuffer &Engine::getIndexStagingBuffer()
{
  auto &buffers = indexStagingBuffers[currentFrame];
  if (currentBufferIndex == buffers.size())
  {
    Logger::info("Creating index staging buffer");

    auto buffer = Buffer::create(
        getIndexBufferSize(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    buffers.push_back(buffer);
  }

  if (currentBufferIndex > buffers.size())
  {
    throw std::runtime_error("current buffer index value out of range");
  }

  auto &isb = buffers[currentBufferIndex];
  return isb;
}

void Engine::queueCurrentBatch()
{
  queueDrawCommand();
  copyStagingBuffersToDevice(currentCommandBuffer);
  unmapStagingBuffers();

  currentBufferIndex++;
}

void Engine::copyStagingBuffersToDevice(VkCommandBuffer commandBuffer)
{

  VkDeviceSize indexSize = (currentIndexWrite - indexWriteStart) * sizeof(uint16_t);
  if (indexSize > 0)
  {
    auto ib = getIndexBuffer();
    auto isb = getIndexStagingBuffer();

    VkBufferCopy copyRegion = {};
    copyRegion.size = indexSize;
    vkCmdCopyBuffer(commandBuffer, isb.buffer, ib.buffer, 1, &copyRegion);
  }

  VkDeviceSize vertexSize = (currentVertexWrite - vertexWriteStart) * sizeof(Vertex);
  if (vertexSize > 0)
  {
    auto vb = getVertexBuffer();
    auto vsb = getVertexStagingBuffer();

    VkBufferCopy copyRegion = {};
    copyRegion.size = vertexSize;
    vkCmdCopyBuffer(commandBuffer, vsb.buffer, vb.buffer, 1, &copyRegion);
  }
}

BoundBuffer &Engine::getVertexBuffer()
{
  auto &buffers = vertexBuffers[currentFrame];
  if (currentBufferIndex == buffers.size())
  {
    Logger::info("Creating vertex buffer");

    auto buffer = Buffer::create(
        getVertexBufferSize(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    buffers.push_back(buffer);
  }

  if (currentBufferIndex > buffers.size())
  {
    throw std::runtime_error("current buffer index value out of range");
  }

  auto &vb = buffers[currentBufferIndex];
  return vb;
}

BoundBuffer &Engine::getIndexBuffer()
{
  auto &buffers = indexBuffers[currentFrame];
  if (currentBufferIndex == buffers.size())
  {
    Logger::info("Creating index buffer");

    auto buffer = Buffer::create(
        getIndexBufferSize(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    buffers.push_back(buffer);
  }

  if (currentBufferIndex > buffers.size())
  {
    throw std::runtime_error("current buffer index value out of range");
  }

  auto &ib = buffers[currentBufferIndex];
  return ib;
}

void Engine::unmapStagingBuffers()
{
  if (!indexWriteStart)
  {
    return;
  }

  auto isb = getIndexStagingBuffer();
  vkUnmapMemory(device, isb.bufferMemory);
  indexWriteStart = nullptr;
  currentIndexWrite = nullptr;
  indexWriteEnd = nullptr;

  auto vsb = getVertexStagingBuffer();
  vkUnmapMemory(device, vsb.bufferMemory);
  vertexWriteStart = nullptr;
  currentVertexWrite = nullptr;
  vertexWriteEnd = nullptr;
}

VkDeviceSize Engine::getVertexBufferSize()
{
  return getMaxNumVertices() * sizeof(Vertex);
}

VkDeviceSize Engine::getIndexBufferSize()
{
  return getMaxNumIndices() * sizeof(uint16_t);
}

VkDeviceSize Engine::getMaxNumIndices()
{
  return 64 * 1024;
}

VkDeviceSize Engine::getMaxNumVertices()
{
  return 64 * 1024;
}

void Engine::drawBatches()
{
  beginRenderPass();

  if (!drawCommands.empty())
  {
    int curBuffer = -1;

    for (auto &cmd : drawCommands)
    {
      if (cmd.bufferIndex != curBuffer)
      {
        curBuffer = cmd.bufferIndex;

        VkBuffer vertexBuffers[] = {this->vertexBuffers[currentFrame][curBuffer].buffer};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(
            currentCommandBuffer,
            0,
            1,
            vertexBuffers,
            offsets);

        vkCmdBindIndexBuffer(
            currentCommandBuffer,
            indexBuffers[currentFrame][curBuffer].buffer,
            0,
            VK_INDEX_TYPE_UINT16);
      }

      std::array<VkDescriptorSet, 2> descriptorSets = {
          perFrameDescriptorSets[currentFrame],
          cmd.descriptorSet};
      vkCmdBindDescriptorSets(currentCommandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout,
                              0,
                              descriptorSets.size(),
                              descriptorSets.data(),
                              0, nullptr);

      vkCmdDrawIndexed(currentCommandBuffer, cmd.numIndices, 1, cmd.baseIndex, 0, 0);
    }
  }
  endRenderPass();

  numDrawCommands = drawCommands.size();
  drawCommands.clear();
}

void Engine::WaitUntilDeviceIdle()
{
  vkDeviceWaitIdle(device);
}

void Engine::cleanupSyncObjects()
{
  for (size_t i = 0; i < getMaxFramesInFlight(); ++i)
  {
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }
}