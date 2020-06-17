#include "swapchain.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <stdexcept>

#include "VulkanHelpers.h"
#include "engine.h"
#include "Logger.h"

void SwapChain::init()
{
  create();
  createImageViews();
}

void SwapChain::create()
{
  Engine *engine = Engine::getInstance();
  SwapChainSupportDetails swapChainSupport = querySupport(engine->getPhysicalDevice(), engine->getSurface());

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  this->extent = chooseSwapExtent(swapChainSupport.capabilities, *engine->getWindow());

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = engine->getSurface();

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = VulkanHelpers::findQueueFamilies(engine->getPhysicalDevice(), engine->getSurface());
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily)
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  if (vkCreateSwapchainKHR(engine->getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(engine->getDevice(), swapChain, &imageCount, nullptr);
  images.resize(imageCount);
  vkGetSwapchainImagesKHR(engine->getDevice(), swapChain, &imageCount, images.data());

  imageFormat = surfaceFormat.format;
}

void SwapChain::recreate()
{
  Logger::info("Recreating swap chain");
  Engine *engine = Engine::getInstance();
  GLFWwindow *window = engine->getWindow();

  vkDeviceWaitIdle(engine->getDevice());

  if (!Engine::isValidWindowSize())
  {
    this->validExtent = false;
    return;
  }

  cleanup();

  create();

  createImageViews();

  engine->createRenderPass();
  engine->createGraphicsPipeline();

  createFramebuffers();

  engine->cleanupSyncObjects();
  engine->createSyncObjects();
  this->validExtent = true;
}

VkSwapchainKHR &SwapChain::get()
{
  return swapChain;
}

SwapChainSupportDetails SwapChain::querySupport(const VkPhysicalDevice &device, const VkSurfaceKHR &surface)
{
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0)
  {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

  if (presentModeCount != 0)
  {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
  for (const auto &availableFormat : availableFormats)
  {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
  for (const auto &availablePresentMode : availablePresentModes)
  {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow &window)
{
  if (capabilities.currentExtent.width != UINT32_MAX)
  {
    return capabilities.currentExtent;
  }
  else
  {
    int width, height;
    glfwGetFramebufferSize(&window, &width, &height);
    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    actualExtent.width = std::max(
        capabilities.minImageExtent.width,
        std::min(capabilities.maxImageExtent.width, actualExtent.width));

    actualExtent.height = std::max(
        capabilities.minImageExtent.height,
        std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

void SwapChain::createImageViews()
{
  Engine *engine = Engine::getInstance();

  imageViews.resize(images.size());

  for (size_t i = 0; i < images.size(); i++)
  {
    imageViews[i] = VulkanHelpers::createImageView(engine->getDevice(), images[i], imageFormat);
  }
}

void SwapChain::createFramebuffers()
{
  Engine *engine = Engine::getInstance();
  framebuffers.resize(imageViews.size());
  engine->getCommandBuffers().resize(imageViews.size());

  for (size_t i = 0; i < imageViews.size(); ++i)
  {
    VkImageView attachments[] = {
        imageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = engine->getRenderPass();
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = engine->getSwapChain().extent.width;
    framebufferInfo.height = engine->getSwapChain().extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(engine->getDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void SwapChain::cleanup()
{
  Engine *engine = Engine::getInstance();
  VkDevice device = engine->getDevice();
  std::vector<VkCommandBuffer> commandBuffers = engine->getCommandBuffers();

  for (auto framebuffer : framebuffers)
  {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  for (size_t i = 0; i < engine->getMaxFramesInFlight(); ++i)
  {
    auto &commandBuffer = engine->getPerFrameCommandBuffer(i);
    if (commandBuffer)
    {
      vkFreeCommandBuffers(
          device,
          engine->getPerFrameCommandPool(i),
          1,
          &commandBuffer);

      commandBuffer = nullptr;
    }
  }

  engine->clearCurrentCommandBuffer();
  vkDestroyPipeline(device, engine->getGraphicsPipeline(), nullptr);
  vkDestroyPipelineLayout(device, engine->getPipelineLayout(), nullptr);
  vkDestroyRenderPass(device, engine->getRenderPass(), nullptr);

  for (auto imageView : imageViews)
  {
    vkDestroyImageView(device, imageView, nullptr);
  }

  vkDestroySwapchainKHR(device, swapChain, nullptr);
}