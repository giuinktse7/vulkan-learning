#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "vertex.h"
#include "../debug.h"

#include "buffer.h"

#include "../item.h"
#include "../position.h"

constexpr uint32_t BATCH_DEVICE_SIZE = 64 * 1024 * sizeof(Vertex);
constexpr uint32_t TILE_SIZE = 32;

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

	Vertex *vertices = nullptr;
	Vertex *current = nullptr;

	uint32_t size;

	std::vector<Batch::DescriptorIndex> descriptorIndices;
	VkDescriptorSet descriptorSet;

	bool isCopiedToDevice = false;

	void setDescriptor(VkDescriptorSet descriptor);

	void addVertex(const Vertex &vertex);

	template <std::size_t SIZE>
	void addVertices(std::array<Vertex, SIZE> &vertices);

	void mapStagingBuffer();
	void unmapStagingBuffer();

	void copyStagingToDevice(VkCommandBuffer commandBuffer);

	const bool canHold(uint32_t vertexCount) const
	{
		return size + vertexCount < BATCH_DEVICE_SIZE;
	}

	const bool isFull() const
	{
		return size == BATCH_DEVICE_SIZE;
	}

private:
};

class BatchDraw
{
public:
	std::vector<Batch> batches;
	VkCommandBuffer commandBuffer;

	BatchDraw();

	void push(Item &item, Position &pos);

	void reset();

	Batch &getBatch();
	void prepareDraw();

private:
	uint32_t batchIndex;

	Batch &getBatch(uint32_t requiredVertexCount);
};