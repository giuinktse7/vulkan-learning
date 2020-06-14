#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>

#include "swapchain.h"

struct Pipeline
{
	Pipeline(SwapChain swapChain);

	VkPipeline vulkanPipeline;

	VkPipelineLayout layout;

	VkDescriptorSetLayout descriptorSetLayout;
	SwapChain &swapChain;

	static void setupDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout *descriptorSetLayout);
	static VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);

	Pipeline &operator=(const Pipeline &other)
	{
		descriptorSetLayout = other.descriptorSetLayout;
		layout = other.layout;
		vulkanPipeline = other.vulkanPipeline;
		return *this;
	}
};