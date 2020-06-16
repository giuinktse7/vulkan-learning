#include "texture.h"

#include <stb_image.h>

#include <cmath>
#include <algorithm>
#include <string>
#include <stdexcept>

#include "engine.h"
#include "VulkanHelpers.h"

#include "buffer.h"

Texture::Texture(const std::string &filename)
{

  int width, height, channels;

  stbi_uc *pixels = stbi_load(filename.c_str(),
                              &width,
                              &height,
                              &channels,
                              STBI_rgb_alpha);

  // mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

  if (!pixels)
  {
    throw std::runtime_error("failed to load texture image!");
  }

  init(static_cast<uint32_t>(width), static_cast<uint32_t>(height), pixels);

  stbi_image_free(pixels);
}

Texture::Texture(uint32_t width, uint32_t height, uint8_t *pixels)
{
  init(width, height, pixels);
}

void Texture::init(uint32_t width, uint32_t height, uint8_t *pixels)
{
  this->width = width;
  this->height = height;
  VkDeviceSize imageSize = width * height * 4;

  Engine *engine = Engine::GetInstance();
  VkDevice device = engine->getDevice();

  auto stagingBuffer = Buffer::create(imageSize,
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  Buffer::copyToMemory(stagingBuffer.bufferMemory, pixels, imageSize);

  // VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

  createImage(width,
              height,
              mipLevels,
              format,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              textureImage,
              textureImageMemory);

  engine->transitionImageLayout(textureImage,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  engine->copyBufferToImage(stagingBuffer.buffer,
                            textureImage,
                            static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height));

  engine->transitionImageLayout(textureImage,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
  vkFreeMemory(device, stagingBuffer.bufferMemory, nullptr);

  //engine->generateMipmaps(textureImage, format, width, height, mipLevels);

  imageView = VulkanHelpers::createImageView(device, textureImage, format);
  sampler = createSampler();
  descriptorSet = createDescriptorSet(imageView, sampler);
}

void Texture::createImage(uint32_t width,
                          uint32_t height,
                          uint32_t mipLevels,
                          VkFormat format,
                          VkImageTiling tiling,
                          VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkImage &image,
                          VkDeviceMemory &imageMemory)
{
  VkDevice device = Engine::GetInstance()->getDevice();
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = mipLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(Engine::GetInstance()->getPhysicalDevice(), memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(device, image, imageMemory, 0);
}

VkSampler Texture::createSampler()
{
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;

  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = 16.0f;

  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.maxLod = static_cast<float>(mipLevels);

  VkSampler sampler;
  if (vkCreateSampler(Engine::GetInstance()->getDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create texture sampler!");
  }

  return sampler;
}

VkDescriptorSet Texture::createDescriptorSet(VkImageView imageView, VkSampler sampler)
{
  Engine *engine = Engine::GetInstance();

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = engine->getDescriptorPool();
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &engine->getPerTextureDescriptorSetLayout();

  VkDescriptorSet descriptorSet;
  if (vkAllocateDescriptorSets(engine->getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate texture descriptor set");
  }

  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = imageView;
  imageInfo.sampler = sampler;

  VkWriteDescriptorSet descriptorWrites = {};
  descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites.dstSet = descriptorSet;
  descriptorWrites.dstBinding = 0;
  descriptorWrites.dstArrayElement = 0;
  descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites.descriptorCount = 1;
  descriptorWrites.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(engine->getDevice(), 1, &descriptorWrites, 0, nullptr);

  return descriptorSet;
}

int Texture::getWidth()
{
  return width;
}

int Texture::getHeight()
{
  return height;
}

VkDescriptorSet Texture::getDescriptorSet()
{
  return descriptorSet;
}

TextureWindow Texture::getTextureWindow()
{
  return TextureWindow{0.0f, 0.0f, 1.0f, 1.0f};
}
