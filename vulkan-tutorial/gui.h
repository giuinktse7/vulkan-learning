#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class GUI
{
public:
	void initialize();
	void startFrame(uint32_t currentFrame);
	void endFrame(uint32_t currentFrame);
	void updateCommandPool(uint32_t currentFrame);

	bool testOpen = true;

	void renderFrame(uint32_t currentFrame);

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