#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

#include "../items.h"

class GUI
{
public:
	void initialize();
	void recordFrame(uint32_t currentFrame);
	void updateCommandPool(uint32_t currentFrame);
	void captureIO();

	void renderItem(ItemType *itemType);

	/* Data */
	uint32_t inputServerId = 4632;
	std::optional<uint16_t> brushServerId;

	uint16_t hoveredId = 0;
	uint16_t nextHoveredId = 0;

	/* End data */

	bool testOpen = true;

	void recreate();

	VkDescriptorPool &getDescriptorPool()
	{
		return descriptorPool;
	}

	VkCommandBuffer getCommandBuffer(uint32_t index)
	{
		return commandBuffers[index];
	}

private:
	void createTopMenuBar();
	void createBottomBar();

	// TODO Replace with actual item list
	void renderN(uint32_t n);

	void initForVulkan();
	void createDescriptorPool();
	void createRenderPass();
	void createFontsTexture();

	void setupCommandPoolsAndBuffers();

	void createFrameBuffers();

	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;

	std::vector<VkCommandPool> commandPools;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkFramebuffer> frameBuffers;

	static void checkVkResult(VkResult err);

	void cleanup();
};