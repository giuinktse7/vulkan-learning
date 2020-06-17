#include "buffer.h"

#include <stdexcept>
#include "VulkanHelpers.h"
#include "engine.h"

BoundBuffer Buffer::create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
  Engine *engine = Engine::getInstance();
  VkDevice device = engine->getDevice();

  VkBuffer buffer;
  VkDeviceMemory bufferMemory;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(engine->getPhysicalDevice(), memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);

  return BoundBuffer{buffer, bufferMemory};
}

void Buffer::copy(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  VkCommandBuffer commandBuffer = Engine::getInstance()->beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  Engine::getInstance()->endSingleTimeCommands(commandBuffer);
}

void Buffer::copyToMemory(VkDeviceMemory bufferMemory, uint8_t *data, VkDeviceSize size)
{
  VkDevice device = Engine::getInstance()->getDevice();
  void *mapped = nullptr;
  vkMapMemory(device, bufferMemory, 0, size, 0, &mapped);
  memcpy(mapped, data, size);
  vkUnmapMemory(device, bufferMemory);
}