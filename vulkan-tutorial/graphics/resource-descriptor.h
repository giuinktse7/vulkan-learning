#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace ResourceDescriptor
{
	VkDescriptorSetLayout createLayout(const VkDevice &device);
	VkDescriptorPool createPool();

} // namespace ResourceDescriptor