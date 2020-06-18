#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_set>

#include <mutex>

#include "DeviceManager.h"
#include "MapRenderer.h"
#include "texture.h"
#include "resource-descriptor.h"
#include "buffer.h"
#include "vertex.h"
#include "camera.h"

enum BlendMode
{
	BM_NONE,
	BM_BLEND,
	BM_ADD,
	BM_ADDX2,

	NUM_BLENDMODES
};

template <typename T>
struct WriteRange
{
	T *start;
	T *cursor;
	T *end;
};

struct RenderInfo
{
	uint16_t indexCount = 0;
	uint16_t indexOffset = 0;
	uint16_t vertexCount = 0;
	uint16_t vertexOffset = 0;

	VkDescriptorSet descriptorSet;
	TextureWindow textureWindow;

	glm::vec4 color;
	BlendMode blendMode;

	WriteRange<uint16_t> indexWrite;
	WriteRange<Vertex> vertexWrite;

	bool isStaged() const
	{
		return indexCount > 0;
	}

	void resetOffset()
	{
		indexCount = 0;
		indexOffset = 0;
		vertexCount = 0;
		vertexOffset = 0;
	}

	void offset()
	{
		indexOffset += indexCount;
		indexCount = 0;
		vertexOffset += vertexCount;
		vertexCount = 0;
	}
};

struct UniformBufferObject
{
	glm::mat4 projection;
};

class Engine
{
protected:
	//Engine()
	//{
	//}
	//~Engine() {}
public:
	Engine();
	~Engine();

	unsigned long frames = 0;

	Camera camera;

	static const int SPRITE_SIZE = 32;

	static Engine *getInstance();
	static void setInstance(Engine *instance);

	void init();

	VkInstance &getVkInstance()
	{
		return instance;
	}

	void setPhysicalDevice(VkPhysicalDevice physicalDevice)
	{
		this->physicalDevice = physicalDevice;
	}

	std::vector<VkCommandBuffer> &getCommandBuffers()
	{
		return commandBuffers;
	}

	GLFWwindow *getWindow()
	{
		return window;
	}

	VkSurfaceKHR &getSurface()
	{
		return surface;
	}

	bool &getFramebufferResized()
	{
		return framebufferResized;
	}

	void setMapRenderer(MapRenderer *renderer)
	{
		mapRenderer = renderer;
	}

	SwapChain &getSwapChain()
	{
		return swapChain;
	}

	VkPhysicalDevice getPhysicalDevice()
	{
		return physicalDevice;
	}

	VkQueue *getGraphicsQueue()
	{
		return mapRenderer->getGraphicsQueue();
	}

	VkQueue *getPresentQueue()
	{
		return mapRenderer->getPresentQueue();
	}

	VkCommandBuffer &getCommandBuffer(uint32_t index)
	{
		return commandBuffers[index];
	}

	void setFrameBufferResized(bool value)
	{
		framebufferResized = value;
	}

	bool hasFrameBufferResized() const
	{
		return framebufferResized;
	}

	VkDevice &getDevice()
	{
		return device;
	}

	VkRenderPass getRenderPass() const
	{
		return renderPass;
	}

	void createRenderPass()
	{
		renderPass = mapRenderer->createRenderPass();
	}

	void createDescriptorPool()
	{
		descriptorPool = ResourceDescriptor::createPool();
	}

	void setRenderPass(VkRenderPass renderPass)
	{
		this->renderPass = renderPass;
	}

	VkDescriptorPool &getDescriptorPool()
	{
		return descriptorPool;
	}

	VkCommandPool getCommandPool()
	{
		return commandPool;
	}

	void clearCurrentCommandBuffer()
	{
		currentCommandBuffer = nullptr;
	}

	VkPipeline &getGraphicsPipeline()
	{
		return graphicsPipeline;
	}

	VkPipelineLayout &getPipelineLayout()
	{
		return pipelineLayout;
	}

	void createCommandPool();

	void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer buffer);

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	std::vector<VkFence> getInFlightFences()
	{
		return inFlightFences;
	};

	std::vector<VkFence> getImagesInFlight()
	{
		return imagesInFlight;
	}

	std::vector<VkSemaphore> getImageAvailableSemaphores()
	{
		return imageAvailableSemaphores;
	};

	VkSemaphore getImageAvailableSemaphore(size_t index)
	{
		return imageAvailableSemaphores[index];
	};

	std::vector<VkSemaphore> getRenderFinishedSemaphores()
	{
		return renderFinishedSemaphores;
	};

	VkSemaphore getRenderFinishedSemaphore(size_t index)
	{
		return renderFinishedSemaphores[index];
	};

	VkCommandBuffer &getPerFrameCommandBuffer(size_t index)
	{
		return perFrameCommandBuffer[index];
	}

	VkCommandPool getPerFrameCommandPool(size_t index)
	{
		return perFrameCommandPool[index];
	}

	void createGraphicsPipeline();

	void allocateCommandBuffers();
	void beginRenderPass();
	void endRenderPass() const;

	void WaitUntilDeviceIdle();

	VkDescriptorSetLayout &getPerTextureDescriptorSetLayout()
	{
		return perTextureDescriptorSetLayout;
	}

	void initialize(GLFWwindow *window);

	void createSyncObjects();
	void cleanupSyncObjects();

	// glm::vec4 getClearColor() const;
	// void setClearColor(const glm::vec4 &clearColor);

	uint32_t getMaxFramesInFlight();

	std::shared_ptr<Texture> CreateTexture(const std::string &filename);

	void setTexture(std::shared_ptr<Texture> texture);

	void drawSprite(float x, float y, float width, float height);

	bool startFrame();
	void endFrame();

	static bool isValidWindowSize();

	void setKeyDown(int key)
	{
		keys.insert(key);
	}

	void setKeyUp(int key)
	{
		keys.erase(key);
	}

	bool isKeyDown(int key) const
	{
		return keys.find(key) != keys.end();
	}

	bool isCtrlDown() const
	{
		return isKeyDown(GLFW_KEY_LEFT_CONTROL) || isKeyDown(GLFW_KEY_RIGHT_CONTROL);
	}

	void setMousePosition(double x, double y)
	{
		this->mousePosition.x = x;
		this->mousePosition.y = y;
	}

	glm::vec2 getMousePosition() const
	{
		return mousePosition;
	}

private:
	glm::vec2 mousePosition;
	bool isInitialized = false;

	static Engine *pinstance_;
	static std::mutex mutex_;

	GLFWwindow *window;
	int width;
	int height;

	VkSurfaceKHR surface;

	VkInstance instance;

	SwapChain swapChain;

	VkDevice device;
	VkPhysicalDevice physicalDevice;

	VkRenderPass renderPass;

	MapRenderer *mapRenderer;

	VkDebugUtilsMessengerEXT debugMessenger;

	std::vector<VkCommandBuffer> commandBuffers;

	bool framebufferResized = false;

	VkCommandPool commandPool;
	std::vector<VkCommandPool> perFrameCommandPool;
	std::vector<VkCommandBuffer> perFrameCommandBuffer;

	VkDescriptorPool descriptorPool;

	std::vector<BoundBuffer> uniformBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	VkDescriptorSetLayout perTextureDescriptorSetLayout;
	VkDescriptorSetLayout perFrameDescriptorSetLayout;

	std::vector<VkDescriptorSet> perFrameDescriptorSets;

	std::vector<std::vector<BoundBuffer>> indexBuffers;
	std::vector<std::vector<BoundBuffer>> indexStagingBuffers;
	std::vector<std::vector<BoundBuffer>> vertexBuffers;
	std::vector<std::vector<BoundBuffer>> vertexStagingBuffers;

	RenderInfo currentRenderInfo;

	std::shared_ptr<Texture> defaultTexture;

	int currentBufferIndex = 0;

	VkCommandBuffer currentCommandBuffer;

	uint32_t currentFrame;
	uint32_t nextFrame;

	glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	// Pipeline
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline = {};

	UniformBufferObject uniformBufferObject;

	std::unordered_set<int> keys;

	struct DrawCommand
	{
		DrawCommand(VkDescriptorSet descriptorSet, uint16_t baseIndex, uint16_t indexCount, int bufferIndex)
				: descriptorSet(descriptorSet),
					baseIndex(baseIndex),
					numIndices(indexCount),
					bufferIndex(bufferIndex)
		{
		}
		VkDescriptorSet descriptorSet;
		uint16_t baseIndex;
		uint16_t numIndices;
		int bufferIndex;
	};

	std::vector<DrawCommand> drawCommands;
	unsigned long drawCommandCount;

	void initWindow();

	void createVulkanInstance();

	static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

	static bool checkValidationLayerSupport();
	static std::vector<const char *> getRequiredExtensions();
	static bool chronosOrStandardValidation(std::vector<VkLayerProperties> &props);
	void createSurface();

	void setSurface(VkSurfaceKHR &surface)
	{
		this->surface = surface;
	}

	void setWindow(GLFWwindow *window)
	{
		this->window = window;
	}

	void createDescriptorSetLayouts();
	VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);
	void createUniformBuffers();

	void createPerFrameDescriptorSets();

	void createIndexAndVertexBuffers();

	void startMainCommandBuffer();

	void updateUniformBuffer();

	bool queueDrawCommand();

	void drawTriangles(const uint16_t *indices, size_t numIndices, const Vertex *vertices, size_t numVertices);

	void mapStagingBufferMemory();

	BoundBuffer &getVertexBuffer();
	BoundBuffer &getIndexBuffer();

	BoundBuffer &getVertexStagingBuffer();
	BoundBuffer &getIndexStagingBuffer();

	VkDeviceSize getMaxNumIndices();
	VkDeviceSize getMaxNumVertices();

	VkDeviceSize getVertexBufferSize();
	VkDeviceSize getIndexBufferSize();

	void queueCurrentBatch();
	void copyStagingBuffersToDevice(VkCommandBuffer commandBuffer);

	void unmapStagingBuffers();

	void drawBatches();
};