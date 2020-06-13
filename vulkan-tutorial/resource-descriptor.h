#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "sprite.h"

namespace ResourceDescriptor
{
	VkDescriptorSetLayout createLayout(const VkDevice &device);
	VkDescriptorPool createPool();

	void createDescriptorSets(Sprite &sprite);
} // namespace ResourceDescriptor