#pragma once

#include <vulkan/vulkan.h>

class GUI
{
public:
	void initialize();

private:
	void initForVulkan();
	void createDescriptorPool();
	void createRenderPass();
	void createFontsTexture();

	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;

	static void checkVkResult(VkResult err);
};