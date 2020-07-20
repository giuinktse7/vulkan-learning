#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct BoundBuffer
{
	VkBuffer buffer = nullptr;
	VkDeviceMemory deviceMemory = nullptr;
};

namespace Buffer
{
	BoundBuffer create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void copy(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void copyToMemory(VkDeviceMemory bufferMemory, uint8_t *data, VkDeviceSize size);
} // namespace Buffer