#include "sprite.h"

#include "buffer.h"
#include "engine.h"

Sprite::Sprite(Texture &texture, std::vector<Vertex> vertices, std::vector<uint16_t> indices)
    : texture(texture),
      vertices(vertices),
      indices(indices),
      pipeline(Pipeline(Engine::GetInstance()->getSwapChain()))
{
}

void Sprite::createBuffers()
{
  createVertexBuffer();
  createIndexBuffer();
  createUniformBuffers();
}

void Sprite::createVertexBuffer()
{
  VkDevice device = Engine::GetInstance()->getDevice();
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  BoundBuffer stagingBuffer = Buffer::create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void *data;
  vkMapMemory(device, stagingBuffer.bufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBuffer.bufferMemory);

  BoundBuffer vertexBuffer = Buffer::create(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  Buffer::copy(stagingBuffer.buffer, vertexBuffer.buffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
  vkFreeMemory(device, stagingBuffer.bufferMemory, nullptr);
}

void Sprite::createIndexBuffer()
{
  VkDevice device = Engine::GetInstance()->getDevice();
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  BoundBuffer stagingBuffer = Buffer::create(bufferSize,
                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void *data;
  vkMapMemory(device, stagingBuffer.bufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBuffer.bufferMemory);

  BoundBuffer indexBuffer = Buffer::create(bufferSize,
                                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  Buffer::copy(stagingBuffer.buffer, indexBuffer.buffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
  vkFreeMemory(device, stagingBuffer.bufferMemory, nullptr);
}

void Sprite::createUniformBuffers()
{
  Engine *engine = Engine::GetInstance();
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  size_t imageCount = engine->getSwapChain().getImages().size();
  uniformBuffers.resize(imageCount);
  uniformBuffersMemory.resize(imageCount);

  for (size_t i = 0; i < imageCount; i++)
  {
    BoundBuffer buffer = Buffer::create(bufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    uniformBuffers[i] = buffer.buffer;
    uniformBuffersMemory[i] = buffer.bufferMemory;
  }
}