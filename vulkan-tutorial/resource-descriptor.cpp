#include "resource-descriptor.h"

#include <array>
#include <stdexcept>

#include "engine.h"

#define MAX_NUM_TEXTURES 256

VkDescriptorSetLayout ResourceDescriptor::createLayout(const VkDevice &device)
{
  VkDescriptorSetLayout layout;
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.pImmutableSamplers = nullptr;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }

  return layout;
}

VkDescriptorPool ResourceDescriptor::createPool()
{
  Engine *engine = Engine::getInstance();

  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  size_t descriptorCount = engine->getMaxFramesInFlight() * 2;

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = descriptorCount;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MAX_NUM_TEXTURES;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = descriptorCount + MAX_NUM_TEXTURES;

  VkDescriptorPool pool;
  if (vkCreateDescriptorPool(engine->getDevice(), &poolInfo, nullptr, &pool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  return pool;
}