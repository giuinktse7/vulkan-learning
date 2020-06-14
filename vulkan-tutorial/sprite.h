#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#include "texture.h"
#include "vertex.h"
#include "pipeline.h"

class Sprite
{
public:
	Sprite(Texture &texture, std::vector<Vertex> vertices, std::vector<uint16_t> indices);

	void createBuffers();

	VkBuffer getUniformBuffer(size_t index)
	{
		return uniformBuffers[index];
	}

	VkDeviceMemory getUniformBufferMemory(size_t index)
	{
		return uniformBuffersMemory[index];
	}

	VkImageView &getTextureImageView()
	{
		return texture.getImageView();
	}

	VkSampler &getTextureSampler()
	{
		return texture.getSampler();
	}

	VkBuffer getVertexBuffer() const
	{
		return vertexBuffer;
	}

	VkBuffer getIndexBuffer() const
	{
		return indexBuffer;
	}

	std::vector<uint16_t> getIndices() const
	{
		return indices;
	}

	std::vector<VkDescriptorSet> &getDescriptorSets()
	{
		return descriptorSets;
	}

	void setDescriptorSets()
	{
	}

	VkDescriptorSet &getDescriptorSet(size_t index)
	{
		return descriptorSets[index];
	}

	void createUniformBuffers();

	Pipeline pipeline;

private:
	Texture &texture;
	std::vector<Vertex> vertices;
	const std::vector<uint16_t> indices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	std::vector<VkDescriptorSet> descriptorSets;

	void createVertexBuffer();

	void createIndexBuffer();
};