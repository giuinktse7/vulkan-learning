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

#include "map.h"

struct Viewport
{
	int width;
	int height;
	float zoom;

	uint32_t offsetX;
	uint32_t offsetY;
};

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

template <typename T>
struct WriteRange
{
	T *start;
	T *cursor;
	T *end;
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
	MapRenderer(std::unique_ptr<Map> map);
	static const int MAX_NUM_TEXTURES = 256 * 256;

	static const int TILE_SIZE = 32;
	static const uint32_t MAX_VERTICES = 64 * 1024;

	Viewport viewport;

	std::unique_ptr<Map> map;

	Camera camera;

	BoundBuffer indexBuffer;

	void initialize();
	void recreate();

	void recordFrame(uint32_t currentFrame);

	void addTextureAtlas(std::unique_ptr<TextureAtlas> &atlas);

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

	void loadTextureAtlases();

	void drawItem(Item &item, Position position);
	void drawItem(Item &item, Position position, glm::vec4 color);

	TextureAtlas *getTextureAtlas(ItemType &itemType);

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

	std::shared_ptr<Texture> defaultTexture;

	std::unordered_map<uint32_t, std::unique_ptr<TextureAtlas>> textureAtlases;

	/* 
		Used for quick retrieval of a texture atlas given a sprite ID.
		It stores the upper bound of the sprite ids in the sprite sheet.
	*/
	std::set<uint32_t> textureAtlasIds;

	TextureAtlas *getTextureAtlas(const uint32_t spriteId);

	void createGraphicsPipeline();
	void createCommandPool();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSetLayouts();
	void createDescriptorSets();
	void createFrameBuffers();

	void updateViewport();
	void updateUniformBuffer();

	void startCommandBuffer();
	void beginRenderPass();

	void drawMap();
	void drawBatches();

	void cleanup();
};