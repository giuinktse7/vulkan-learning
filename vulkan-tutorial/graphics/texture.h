#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <filesystem>

#include <memory>

#include "device_manager.h"

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
  Texture(uint32_t width, uint32_t height, std::vector<uint8_t> pixels);

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

  static Texture *getBlackTexture();

private:
  void init(uint32_t width, uint32_t height, uint8_t *pixels);

  static std::unique_ptr<Texture> blackSquare;

  uint32_t width;
  uint32_t height;

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;

  VkImageView imageView;
  VkSampler sampler;

  VkDescriptorSet descriptorSet;

  uint32_t mipLevels = 1;

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
