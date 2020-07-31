#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "vertex.h"
#include "../debug.h"

#include "buffer.h"

#include "../item.h"
#include "../position.h"

constexpr uint32_t BATCH_DEVICE_SIZE = 4 * 128 * sizeof(Vertex);
// constexpr uint32_t BATCH_DEVICE_SIZE = 4 * 144 * sizeof(Vertex);

struct Batch
{
	struct DescriptorIndex
	{
		VkDescriptorSet descriptor;
		uint32_t end;
	};

	Batch();
	~Batch();
	BoundBuffer buffer;
	BoundBuffer stagingBuffer;

	Batch(Batch &&other)
	{
		this->valid = other.valid;
		this->isCopiedToDevice = other.isCopiedToDevice;
		this->buffer = other.buffer;
		this->stagingBuffer = other.stagingBuffer;
		this->descriptorIndices = other.descriptorIndices;
		this->descriptorSet = other.descriptorSet;
		this->vertexCount = other.vertexCount;
		this->vertices = other.vertices;
		this->current = other.current;

		other.vertexCount = 0;
		other.stagingBuffer.buffer = nullptr;
		other.stagingBuffer.deviceMemory = nullptr;
		other.buffer.buffer = nullptr;
		other.buffer.deviceMemory = nullptr;
		other.descriptorSet = nullptr;
		other.vertices = nullptr;
		other.current = nullptr;
	}

	Vertex *vertices = nullptr;
	Vertex *current = nullptr;

	uint32_t vertexCount = 0;

	std::vector<Batch::DescriptorIndex> descriptorIndices;
	VkDescriptorSet descriptorSet;

	bool isCopiedToDevice = false;

	void setDescriptor(VkDescriptorSet descriptor);

	void addVertex(const Vertex &vertex);

	template <std::size_t SIZE>
	void addVertices(std::array<Vertex, SIZE> &vertices);

	void reset();

	void mapStagingBuffer();
	void unmapStagingBuffer();

	const bool isValid() const
	{
		return this->valid;
	}

	void copyStagingToDevice(VkCommandBuffer commandBuffer);

	const bool canHold(uint32_t vertexCount) const
	{
		return (this->vertexCount + vertexCount) * sizeof(Vertex) < BATCH_DEVICE_SIZE;
	}

	void invalidate();

	friend class BatchDraw;

private:
	// Flag to signal whether recreation (i.e. re-mapping) is necessary.
	bool valid = true;
};

class BatchDraw
{
public:
	std::vector<Batch> batches;
	VkCommandBuffer commandBuffer;

	BatchDraw();

	void push(Item &item, Position &pos, glm::vec4 color);
	void push(Item &item, Position &pos);

	void push(ItemType &itemType, TextureWindow textureWindow, Position &pos);
	void push(ItemType &itemType, TextureWindow window, Position &pos, glm::vec4 color);

	void reset();

	Batch &getBatch();
	void prepareDraw();

private:
	uint32_t batchIndex;

	Batch &getBatch(uint32_t requiredVertexCount);
};