#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <mutex>

#include "DeviceManager.h"
#include "MapRenderer.h"
#include "sprite.h"
#include "resource-descriptor.h"
#include "bound-buffer.h"

enum BlendMode
{
	BM_NONE,
	BM_BLEND,
	BM_ADD,
	BM_ADDX2,

	NUM_BLENDMODES
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

	static Engine *GetInstance();
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

	void addSprite(Sprite sprite)
	{
		sprites.push_back(sprite);
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

	void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer buffer);

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	void createCommandBuffers();

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

	std::vector<Sprite> &getSprites()
	{
		return sprites;
	}

	std::vector<VkCommandBuffer> &getPerFrameCommandBuffer(size_t index)
	{
		return perFrameCommandBuffer[index];
	}

	VkCommandPool getPerFrameCommandPool(size_t index)
	{
		return perFrameCommandPool[index];
	}

	void createGraphicsPipeline();

	void beginRenderCommands();
	void endRenderCommands();
	void allocateCommandBuffers();
	void beginRenderPass();
	void endRenderPass() const;
	void renderSprites(size_t bufferIndex);
	void recordCommands();

	void WaitUntilDeviceIdle();

	VkDescriptorSetLayout &getPerTextureDescriptorSetLayout()
	{
		return perTextureDescriptorSetLayout;
	}

	void initialize(GLFWwindow *window);

	void createSyncObjects();
	void cleanupSyncObjects();

	glm::vec4 getClearColor() const;
	void setClearColor(const glm::vec4 &clearColor);

	uint32_t getMaxFramesInFlight();

	std::shared_ptr<Texture> CreateTexture(const std::string &filename);

	void SetTexture(std::shared_ptr<Texture> texture);

	void DrawSprite(float x, float y, float width, float height);

	bool StartFrame();
	void EndFrame();

private:
	bool isInitialized = false;

	static Engine *pinstance_;
	static std::mutex mutex_;

	std::vector<Sprite> sprites;

	GLFWwindow *window;

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
	std::vector<std::vector<VkCommandBuffer>> perFrameCommandBuffer;

	VkDescriptorPool descriptorPool;

	std::vector<What::BoundBuffer> uniformBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	VkDescriptorSetLayout perTextureDescriptorSetLayout;
	VkDescriptorSetLayout perFrameDescriptorSetLayout;

	std::vector<VkDescriptorSet> perFrameDescriptorSets;

	std::vector<std::vector<What::BoundBuffer>> indexBuffers;
	std::vector<std::vector<What::BoundBuffer>> indexStagingBuffers;
	std::vector<std::vector<What::BoundBuffer>> vertexBuffers;
	std::vector<std::vector<What::BoundBuffer>> vertexStagingBuffers;

	uint16_t *indexWriteStart;
	uint16_t *currentIndexWrite;
	uint16_t *indexWriteEnd;
	Vertex *vertexWriteStart;
	Vertex *currentVertexWrite;
	Vertex *vertexWriteEnd;

	uint16_t numIndices = 0;
	uint16_t indexOffset = 0;
	uint16_t numVertices = 0;
	uint16_t vertexOffset = 0;

	std::shared_ptr<Texture> defaultTexture;

	// Current
	VkDescriptorSet currentDescriptorSet;
	TextureWindow currentTextureWindow;

	glm::vec4 currentColor;
	BlendMode currentBlendMode;

	int currentBufferIndex = 0;

	VkCommandBuffer currentCommandBuffer;

	uint32_t currentFrame;
	uint32_t nextFrame;

	glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	// Pipeline
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline = {};

	struct DrawCommand
	{
		DrawCommand(VkDescriptorSet ds, uint16_t bi, uint16_t ni, int buf) : descriptorSet(ds),
																																				 baseIndex(bi),
																																				 numIndices(ni),
																																				 bufferIndex(buf)
		{
		}
		VkDescriptorSet descriptorSet;
		uint16_t baseIndex;
		uint16_t numIndices;
		int bufferIndex;
	};

	std::vector<DrawCommand> drawCommands;
	unsigned long numDrawCommands;

	void initWindow();

	void createInstance();

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

	void renderSprite(size_t bufferIndex, Sprite &sprite);

	void createDescriptorSetLayouts();
	VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);
	void createUniformBuffers();

	void createPerFrameDescriptorSets();

	void createIndexAndVertexBuffers();

	void startMainCommandBuffer();

	void updateUniformBuffer() const;

	bool queueDrawCommand();

	void DrawTriangles(const uint16_t *indices, size_t numIndices, const Vertex *vertices, size_t numVertices);

	void mapStagingBufferMemory();

	What::BoundBuffer &getVertexBuffer();
	What::BoundBuffer &getIndexBuffer();

	What::BoundBuffer &getVertexStagingBuffer();
	What::BoundBuffer &getIndexStagingBuffer();

	VkDeviceSize getMaxNumIndices();
	VkDeviceSize getMaxNumVertices();

	VkDeviceSize getVertexBufferSize();
	VkDeviceSize getIndexBufferSize();

	void queueCurrentBatch();
	void copyStagingBuffersToDevice(VkCommandBuffer commandBuffer);

	void unmapStagingBuffers();

	void drawBatches();
};