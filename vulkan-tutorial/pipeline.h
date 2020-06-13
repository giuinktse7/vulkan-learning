#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "swapchain.h"

struct Pipeline
{
	Pipeline(SwapChain swapChain);

	VkPipeline vulkanPipeline;

	VkPipelineLayout layout;

	VkDescriptorSetLayout descriptorSetLayout;
	SwapChain &swapChain;

	static Pipeline createItemPipeline();
};