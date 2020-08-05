#include "batch_item_draw.h"

#include "engine.h"
#include <ios>

BatchDraw::BatchDraw()
    : batchIndex(0), commandBuffer(nullptr)
{
}

Batch::~Batch()
{
  if (stagingBuffer.buffer != VK_NULL_HANDLE)
  {
    DEBUG_ASSERT(stagingBuffer.deviceMemory != VK_NULL_HANDLE, "Non-null buffer with null deviceMemory. This should not happen.");
    vkDestroyBuffer(g_engine->getDevice(), stagingBuffer.buffer, nullptr);
    vkFreeMemory(g_engine->getDevice(), stagingBuffer.deviceMemory, nullptr);
  }
}

Batch::Batch()
    : vertexCount(0), descriptorSet(nullptr), valid(true)
{
  this->stagingBuffer = Buffer::create(
      BATCH_DEVICE_SIZE,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // std::cout << "Creating buffer at 0x" << std::hex << this->stagingBuffer.deviceMemory << std::endl;

  this->buffer = Buffer::create(
      BATCH_DEVICE_SIZE,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  mapStagingBuffer();
}

void Batch::invalidate()
{
  this->valid = false;
}

void Batch::reset()
{
  this->vertexCount = 0;
  this->descriptorIndices.clear();
  this->descriptorSet = nullptr;
  this->vertices = nullptr;
  this->current = nullptr;
  this->valid = true;
}

void Batch::mapStagingBuffer()
{
  void *data;
  g_engine->mapMemory(this->stagingBuffer.deviceMemory, 0, BATCH_DEVICE_SIZE, 0, &data);
  this->vertices = reinterpret_cast<Vertex *>(data);
  this->current = vertices;

  this->isCopiedToDevice = false;
}

void Batch::unmapStagingBuffer()
{
  DEBUG_ASSERT(current != nullptr, "Tried to unmap an item batch that has not been mapped.");

  vkUnmapMemory(g_engine->getDevice(), this->stagingBuffer.deviceMemory);
  this->vertices = nullptr;
  this->current = nullptr;
}

void Batch::copyStagingToDevice(VkCommandBuffer commandBuffer)
{
  DEBUG_ASSERT(this->isCopiedToDevice == false, "The staging buffer has alreday been copied to the device.");
  this->isCopiedToDevice = true;

  if (vertexCount == 0)
    return;

  VkBufferCopy copyRegion = {};
  copyRegion.size = this->vertexCount * sizeof(Vertex);
  vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer.buffer, 1, &copyRegion);
}

void BatchDraw::push(ObjectDrawInfo &info)
{
  auto [appearance, textureInfo, position, color] = info;
  auto [atlas, window] = textureInfo;

  auto drawOffset = atlas->drawOffset;

  int worldX = g_engine->gameToWorldPos(position.x + drawOffset.x);
  int worldY = g_engine->gameToWorldPos(position.y + drawOffset.y);

  // Add the item shift if necessary
  if (appearance->hasFlag(AppearanceFlag::Shift))
  {
    worldX -= appearance->flagData.shiftX;
    worldY -= appearance->flagData.shiftY;
  }

  uint32_t width = atlas->spriteWidth;
  uint32_t height = atlas->spriteHeight;

  glm::vec2 atlasSize = {atlas->width, atlas->height};
  const float offsetX = 0.5f / atlasSize.x;
  const float offsetY = 0.5f / atlasSize.y;

  const glm::vec4 rect = {
      window.x0 + offsetX,
      window.y0 - offsetY,
      window.x1 - offsetX,
      window.y1 + offsetY};

  Batch &batch = getBatch(4);
  batch.setDescriptor(atlas->getDescriptorSet());

  std::array<Vertex, 4> vertices{{
      {{worldX, worldY}, color, {window.x0, window.y0}, rect},
      {{worldX, worldY + height}, color, {window.x0, window.y1}, rect},
      {{worldX + width, worldY + height}, color, {window.x1, window.y1}, rect},
      {{worldX + width, worldY}, color, {window.x1, window.y0}, rect},
  }};

  batch.addVertices(vertices);
}

template <std::size_t SIZE>
void Batch::addVertices(std::array<Vertex, SIZE> &vertexArray)
{
  memcpy(this->current, &vertexArray, sizeof(vertexArray));
  current += vertexArray.size();
  vertexCount += static_cast<uint32_t>(vertexArray.size());
}

Batch &BatchDraw::getBatch() const
{
  // Request no additional space
  return getBatch(0);
}

Batch &BatchDraw::getBatch(uint32_t requiredVertexCount) const
{
  if (batches.empty())
  {
    batches.emplace_back();
  }

  Batch &batch = batches.at(batchIndex);
  size_t amount = batches.size();

  if (!batch.valid)
  {
    batch.reset();
    batch.mapStagingBuffer();
  }

  if (!batch.canHold(requiredVertexCount))
  {
    batch.setDescriptor(nullptr);
    batch.copyStagingToDevice(this->commandBuffer);
    batch.unmapStagingBuffer();

    ++batchIndex;
    if (batchIndex == batches.size())
      batches.emplace_back();
  }

  Batch &resultBatch = batches.at(batchIndex);

  if (!resultBatch.valid)
  {
    resultBatch.reset();
    resultBatch.mapStagingBuffer();
  }

  return resultBatch;
}

void BatchDraw::reset()
{
  this->batchIndex = 0;
}

void Batch::addVertex(const Vertex &vertex)
{
  *current = vertex;
  ++current;
  ++vertexCount;
}

void Batch::setDescriptor(VkDescriptorSet descriptor)
{
  if (this->descriptorSet && this->descriptorSet != descriptor)
  {
    descriptorIndices.emplace_back<Batch::DescriptorIndex>({this->descriptorSet, vertexCount - 1});
  }

  this->descriptorSet = descriptor;
}

void BatchDraw::prepareDraw()
{
  // std::cout << "prepareDraw() batches: " << batchIndex << std::endl;
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

  this->batchIndex = 0;
}

std::vector<Batch> &BatchDraw::getBatches() const
{
  return batches;
}