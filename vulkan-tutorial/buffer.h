#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "bound-buffer.h"

namespace Buffer
{
	What::BoundBuffer create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void copy(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void copyToMemory(VkDeviceMemory bufferMemory, uint8_t *data, VkDeviceSize size);
} // namespace Buffer