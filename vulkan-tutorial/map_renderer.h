#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <memory>

#include "graphics/buffer.h"
#include "graphics/vertex.h"
#include "graphics/texture.h"
#include "camera.h"

#include "graphics/swapchain.h"

struct ItemUniformBufferObject
{
	glm::mat4 projection;
};

enum BlendMode
{
	BM_NONE,
	BM_BLEND,
	BM_ADD,
	BM_ADDX2,

	NUM_BLENDMODES
};

template <typename T>
struct WriteRange
{
	T *start;
	T *cursor;
	T *end;
};

struct RenderInfo
{
	uint16_t indexCount = 0;
	uint16_t indexOffset = 0;
	uint16_t vertexCount = 0;
	uint16_t vertexOffset = 0;

	VkDescriptorSet descriptorSet;
	TextureWindow textureWindow;

	glm::vec4 color;
	BlendMode blendMode;

	WriteRange<uint16_t> indexWrite;
	WriteRange<Vertex> vertexWrite;

	bool isStaged() const
	{
		return indexCount > 0;
	}

	void resetOffset()
	{
		indexCount = 0;
		indexOffset = 0;
		vertexCount = 0;
		vertexOffset = 0;
	}

	void offset()
	{
		indexOffset += indexCount;
		indexCount = 0;
		vertexOffset += vertexCount;
		vertexCount = 0;
	}
};

struct RenderData
{
	RenderInfo info;

	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkDescriptorSet> descriptorSets;

	ItemUniformBufferObject uniformBufferObject;
	std::vector<BoundBuffer> uniformBuffers;

	std::vector<std::vector<BoundBuffer>> indexBuffers;
	std::vector<std::vector<BoundBuffer>> indexStagingBuffers;
	std::vector<std::vector<BoundBuffer>> vertexBuffers;
	std::vector<std::vector<BoundBuffer>> vertexStagingBuffers;

	uint32_t index = 0;

	std::vector<BoundBuffer> &getVertexBuffers()
	{
		return vertexBuffers[index];
	}

	std::vector<BoundBuffer> &getIndexBuffers()
	{
		return indexBuffers[index];
	}

	std::vector<BoundBuffer> &getVertexStagingBuffers()
	{
		return vertexStagingBuffers[index];
	}

	std::vector<BoundBuffer> &getIndexStagingBuffers()
	{
		return indexStagingBuffers[index];
	}

	VkFramebuffer &getFrameBuffer()
	{
		return frameBuffers[index];
	}

	VkCommandBuffer &getCommandBuffer()
	{
		return commandBuffers[index];
	}

	VkDescriptorSet getDescriptorSet()
	{
		return descriptorSets[index];
	}

	BoundBuffer &getUniformBuffer()
	{
		return uniformBuffers[index];
	}

	void setCurrentIndex(uint32_t index)
	{
		this->index = index;
	}

	void resize(uint32_t size)
	{
		frameBuffers.resize(size);
		commandBuffers.resize(size);
		uniformBuffers.resize(size);
	}
};

class MapRenderer
{
public:
	static const int MAX_NUM_TEXTURES = 256;

	static const int TILE_SIZE = 32;
	static const uint32_t MAX_VERTICES = 64 * 1024;

	Camera camera;

	void initialize();
	void recreate();

	void renderFrame(uint32_t currentFrame);
	void endFrame();

	void setTexture(std::shared_ptr<Texture> texture);

	VkCommandBuffer &getCommandBuffer()
	{
		return renderData.getCommandBuffer();
	}

	VkRenderPass createRenderPass();
	void setCurrentFrame(uint32_t frame)
	{
		this->renderData.setCurrentIndex(frame);
	}

	VkDescriptorPool &getDescriptorPool()
	{
		return descriptorPool;
	}

	VkDescriptorSetLayout &getTextureDescriptorSetLayout()
	{
		return textureDescriptorSetLayout;
	}

	void drawSprite(float x, float y, float width, float height);

private:
	VkRenderPass renderPass;

	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline = {};

	RenderData renderData;
	VkDescriptorSetLayout frameDescriptorSetLayout;
	VkDescriptorSetLayout textureDescriptorSetLayout;

	const glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	std::shared_ptr<Texture> defaultTexture;

	struct DrawCommand
	{
		DrawCommand(VkDescriptorSet descriptorSet, uint16_t baseIndex, uint16_t indexCount, int bufferIndex)
				: descriptorSet(descriptorSet),
					baseIndex(baseIndex),
					numIndices(indexCount),
					bufferIndex(bufferIndex)
		{
		}
		VkDescriptorSet descriptorSet;
		uint16_t baseIndex;
		uint16_t numIndices;
		int bufferIndex;
	};

	std::vector<DrawCommand> drawCommands;
	int currentBufferIndex = 0;

	VkFramebuffer &getFrameBuffer()
	{
		return renderData.getFrameBuffer();
	}

	BoundBuffer &getUniformBuffer()
	{
		return renderData.getUniformBuffer();
	}

	void createGraphicsPipeline();
	void createCommandPool();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSetLayouts();
	void createDescriptorSets();
	void createFrameBuffers();
	void updateUniformBuffer();

	void startCommandBuffer();
	void beginRenderPass();
	void endRenderPass();

	void mapStagingBufferMemory();

	BoundBuffer &getVertexBuffer();
	BoundBuffer &getIndexBuffer();

	BoundBuffer &getVertexStagingBuffer();
	BoundBuffer &getIndexStagingBuffer();

	VkDeviceSize getMaxNumIndices();
	VkDeviceSize getMaxNumVertices();

	VkDeviceSize getVertexBufferSize();
	VkDeviceSize getIndexBufferSize();

	void queueCurrentBatch();
	void queueDrawCommand();
	void copyStagingBuffersToDevice();
	void unmapStagingBuffers();

	void drawBatches();

	void drawTriangles(const uint16_t *indices, size_t indexCount, const Vertex *vertices, size_t vertexCount);

	void cleanup();
};