#pragma once

/**
Vulkan does not have the concept of a "default framebuffer", hence it requires an
infrastructure that will own the buffers we will render to before we visualize
them on the screen. This infrastructure is known as the swap chain and must be
created explicitly in Vulkan. The swap chain is essentially a queue of images
that are waiting to be presented to the screen. Our application will acquire such
an image to draw to it, and then return it to the queue. How exactly the queue
works and the conditions for presenting an image from the queue depend on how
the swap chain is set up, but the general purpose of the swap chain is to
synchronize the presentation of images with the refresh rate of the screen.
*/

#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "DeviceManager.h"
#include "VulkanHelpers.h"

class SwapChain
{
public:
  static SwapChainSupportDetails querySupport(const VkPhysicalDevice &device, const VkSurfaceKHR &surface);

  void initialize();
  void create();
  void recreate();

  VkSwapchainKHR &get();

  void createFramebuffers();

  VkFormat getImageFormat()
  {
    return imageFormat;
  }

  std::vector<VkImage> &getImages()
  {
    return images;
  }

  VkExtent2D getExtent() const
  {
    return extent;
  }

  std::vector<VkFramebuffer> &getFramebuffers()
  {
    return framebuffers;
  }

  VkFramebuffer &getFrameBuffer(size_t index)
  {
    return framebuffers[index];
  }

  bool isValid() const
  {
    return validExtent;
  }

  uint32_t getImageCount()
  {
    return imageCount;
  }

  uint32_t getImageViewCount()
  {
    return imageViews.size();
  }

  uint32_t getMinImageCount()
  {
    return minImageCount;
  }

  VkImageView getImageView(uint32_t index)
  {
    return imageViews[index];
  }

private:
  bool validExtent = true;
  uint32_t minImageCount;
  uint32_t imageCount;

  VkSwapchainKHR swapChain;
  std::vector<VkImage> images;

  std::vector<VkImageView> imageViews;

  std::vector<VkFramebuffer> framebuffers;

  VkFormat imageFormat;
  VkExtent2D extent;

  static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow &window);
  static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
  static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

  void createImageViews();

  void cleanup();
};