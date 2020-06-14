#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

#include "DeviceManager.h"

struct TextureWindow
{
  float x0;
  float y0;
  float x1;
  float y1;
};

class Texture
{
public:
  Texture(const std::string &filename);
  Texture(uint32_t width, uint32_t height, uint8_t *pixels);

  VkDescriptorSet getDescriptorSet();
  TextureWindow getTextureWindow();
  int getWidth();
  int getHeight();

  VkSampler createSampler();

  VkImageView &getImageView()
  {
    return imageView;
  }

  VkSampler &getSampler()
  {
    return sampler;
  }

private:
  void init(uint32_t width, uint32_t height, uint8_t *pixels);

  uint32_t width;
  uint32_t height;

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;

  VkImageView imageView;
  VkSampler sampler;

  VkDescriptorSet descriptorSet;

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

  VkDescriptorSet createDescriptorSet(VkImageView imageView, VkSampler sampler);
};