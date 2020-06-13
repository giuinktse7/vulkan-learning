#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Buffer
{
	void create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
	void copy(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
} // namespace Buffer