#pragma once

#include <vector>
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "swapchain.h"

class MapRenderer
{
public:
	void init();
	void drawFrame();

	VkQueue *getGraphicsQueue()
	{
		return &graphicsQueue;
	}

	VkQueue *getPresentQueue()
	{
		return &presentQueue;
	}

	VkRenderPass createRenderPass();

private:
	const int MAX_FRAMES_IN_FLIGHT = 2;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	size_t currentFrame = 0;

	void updateUniformBuffer(uint32_t currentImage);

	void createDescriptorSetLayout();
};