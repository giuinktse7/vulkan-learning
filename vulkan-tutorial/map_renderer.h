#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <memory>

#include <unordered_map>

#include "graphics/buffer.h"
#include "graphics/vertex.h"
#include "graphics/texture.h"
#include "camera.h"

#include "position.h"

#include "item.h"
#include "items.h"

#include "graphics/swapchain.h"

#include "graphics/texture_atlas.h"

#include "graphics/batch_item_draw.h"
#include "map_view.h"

#include "map.h"

struct TextureOffset
{
	float x;
	float y;
};

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

struct FrameData
{
	VkFramebuffer frameBuffer = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	BoundBuffer uniformBuffer{};
	VkDescriptorSet descriptorSet = nullptr;

	BatchDraw batchDraw{};
};

class MapRenderer
{
public:
	static const int MAX_NUM_TEXTURES = 256 * 256;

	static const int TILE_SIZE = 32;
	static const uint32_t MAX_VERTICES = 64 * 1024;

	// All sprites are drawn using this index buffer
	BoundBuffer indexBuffer;

	void initialize();
	void recreate();

	void recordFrame(uint32_t currentFrame, MapView &mapView);

	VkCommandBuffer getCommandBuffer();

	void createRenderPass();

	VkDescriptorPool &getDescriptorPool()
	{
		return descriptorPool;
	}

	VkDescriptorSetLayout &getTextureDescriptorSetLayout()
	{
		return textureDescriptorSetLayout;
	}

	void drawTile(const TileLocation &tileLocation);

	void drawItem(ObjectDrawInfo &info);

private:
	std::array<FrameData, 3> frames;

	FrameData *currentFrame;

	VkRenderPass renderPass;

	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline = {};

	VkDescriptorSetLayout frameDescriptorSetLayout;
	VkDescriptorSetLayout textureDescriptorSetLayout;

	const glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	void createGraphicsPipeline();
	void createCommandPool();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSetLayouts();
	void createDescriptorSets();
	void createFrameBuffers();

	void updateUniformBuffer(const MapView &mapView);

	void startCommandBuffer();
	void beginRenderPass();

	void drawMap(const MapView &mapView);
	void drawPreviewCursor(const MapView &mapView);

	void drawBatches();

	void cleanup();
};