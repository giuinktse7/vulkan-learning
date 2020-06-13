#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "DeviceManager.h"

class Texture
{
public:
  Texture(const char *filename);

  void createImage();
  void createSampler();

  VkImageView &getImageView()
  {
    return imageView;
  }

  VkSampler &getSampler()
  {
    return sampler;
  }

private:
  const char *filename;

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;

  VkImageView imageView;
  VkSampler sampler;

  uint32_t mipLevels;

  void createImage(uint32_t width,
                   uint32_t height,
                   uint32_t mipLevels,
                   VkFormat format,
                   VkImageTiling tiling,
                   VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   VkImage &image,
                   VkDeviceMemory &imageMemory);
};