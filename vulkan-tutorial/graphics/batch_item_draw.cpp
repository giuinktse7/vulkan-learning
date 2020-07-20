#include "batch_item_draw.h"

#include "engine.h"

constexpr glm::vec4 DEFAULT_COLOR{1.0f, 1.0f, 1.0f, 1.0f};

BatchDraw::BatchDraw()
    : batchIndex(0), commandBuffer(nullptr)
{
}

Batch::~Batch()
{
  auto engine = Engine::getInstance();
  vkDestroyBuffer(engine->getDevice(), stagingBuffer.buffer, nullptr);
  vkFreeMemory(engine->getDevice(), stagingBuffer.deviceMemory, nullptr);
}

Batch::Batch()
    : size(0), descriptorSet(nullptr)
{
  this->stagingBuffer = Buffer::create(
      BATCH_DEVICE_SIZE,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  this->buffer = Buffer::create(
      BATCH_DEVICE_SIZE,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  mapStagingBuffer();
}

void Batch::mapStagingBuffer()
{
  auto engine = Engine::getInstance();

  void *data;
  vkMapMemory(engine->getDevice(), this->stagingBuffer.deviceMemory, 0, BATCH_DEVICE_SIZE, 0, &data);
  this->vertices = reinterpret_cast<Vertex *>(data);
  this->current = vertices;

  this->isCopiedToDevice = false;
}

void Batch::unmapStagingBuffer()
{
  auto engine = Engine::getInstance();

  DEBUG_ASSERT(current != nullptr, "Tried to unmap an item batch that has not been mapped.");

  vkUnmapMemory(engine->getDevice(), this->stagingBuffer.deviceMemory);
  this->vertices = nullptr;
  this->current = nullptr;
}

void Batch::copyStagingToDevice(VkCommandBuffer commandBuffer)
{
  DEBUG_ASSERT(this->isCopiedToDevice == false, "The staging buffer has alreday been copied to the device.");
  this->isCopiedToDevice = true;

  if (size == 0)
    return;

  VkBufferCopy copyRegion = {};
  copyRegion.size = this->size * sizeof(Vertex);
  vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer.buffer, 1, &copyRegion);
}

void BatchDraw::push(Item &item, Position &pos)
{
  auto drawOffset = item.getDrawOffset();

  float worldX = (pos.x + drawOffset.x) * TILE_SIZE;
  float worldY = (pos.y + drawOffset.y) * TILE_SIZE;

  float width = item.getWidth();
  float height = item.getHeight();

  auto window = item.getTextureWindow();

  Batch &batch = getBatch(4);
  batch.setDescriptor(item.itemType->textureAtlas->getDescriptorSet());

  std::array<Vertex, 4> vertices{{
      {{worldX, worldY}, DEFAULT_COLOR, {window.x0, window.y0}},
      {{worldX, worldY + height}, DEFAULT_COLOR, {window.x0, window.y1}},
      {{worldX + width, worldY + height}, DEFAULT_COLOR, {window.x1, window.y1}},
      {{worldX + width, worldY}, DEFAULT_COLOR, {window.x1, window.y0}},
  }};

  batch.addVertices(vertices);
}

template <std::size_t SIZE>
void Batch::addVertices(std::array<Vertex, SIZE> &vertexArray)
{
  memcpy(this->current, &vertexArray, sizeof(vertexArray));
  current += vertexArray.size();
  size += vertexArray.size();
}

Batch &BatchDraw::getBatch()
{
  // Request no additional space
  return getBatch(0);
}

Batch &BatchDraw::getBatch(uint32_t requiredVertexCount)
{
  if (batches.empty())
  {
    batches.emplace_back();
  }

  if (!batches[batchIndex].canHold(requiredVertexCount))
  {
    batches[batchIndex].setDescriptor(nullptr);
    batches[batchIndex].copyStagingToDevice(this->commandBuffer);
    batches[batchIndex].unmapStagingBuffer();

    ++batchIndex;
    if (batchIndex == batches.size())
      batches.emplace_back();
  }

  if (!batches[batchIndex].current)
  {
    batches[batchIndex].mapStagingBuffer();
  }

  return batches[batchIndex];
}

void BatchDraw::reset()
{
  this->batchIndex = 0;
}

void Batch::addVertex(const Vertex &vertex)
{
  *current = vertex;
  ++current;
  ++size;
}

void Batch::setDescriptor(VkDescriptorSet descriptor)
{
  if (this->descriptorSet && this->descriptorSet != descriptor)
  {
    descriptorIndices.emplace_back<Batch::DescriptorIndex>({this->descriptorSet, size - 1});
  }

  this->descriptorSet = descriptor;
}

void BatchDraw::prepareDraw()
{
  Batch &latestBatch = getBatch();
  if (latestBatch.descriptorIndices.empty() || latestBatch.descriptorIndices.back().descriptor != latestBatch.descriptorSet)
  {
    latestBatch.setDescriptor(nullptr);
  }

  if (!latestBatch.isCopiedToDevice)
  {
    latestBatch.copyStagingToDevice(commandBuffer);
    latestBatch.unmapStagingBuffer();
  }
}
