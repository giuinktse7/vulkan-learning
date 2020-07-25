#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class GUI
{
public:
	void initialize();
	void recordFrame(uint32_t currentFrame);
	void endFrame(uint32_t currentFrame);
	void updateCommandPool(uint32_t currentFrame);
	void captureIO();

	void renderItem(uint32_t serverId);

	/* Data */
	uint32_t selectedServerId = 100;
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