#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "vertex.h"
#include "../debug.h"

#include "texture.h"
#include "texture_atlas.h"

#include "buffer.h"

#include "../item.h"
#include "../position.h"

constexpr uint32_t BATCH_DEVICE_SIZE = 4 * 128 * sizeof(Vertex);
// constexpr uint32_t BATCH_DEVICE_SIZE = 4 * 144 * sizeof(Vertex);

struct ObjectDrawInfo
{
	Appearance *appearance;
	TextureInfo textureInfo;
	Position position;
	glm::vec4 color;
};

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
	VkCommandBuffer commandBuffer;

	BatchDraw();

	void push(ObjectDrawInfo &info);

	void reset();

	Batch &getBatch() const;
	std::vector<Batch> &getBatches() const;
	void prepareDraw();

private:
	mutable uint32_t batchIndex;
	mutable std::vector<Batch> batches;

	Batch &getBatch(uint32_t requiredVertexCount) const;
};