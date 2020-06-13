#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <mutex>

#include "DeviceManager.h"
#include "MapRenderer.h"
#include "sprite.h"
#include "resource-descriptor.h"

class Engine
{
protected:
	Engine()
	{
	}
	~Engine() {}

public:
	Engine(Engine &other) = delete;
	void operator=(const Engine &) = delete;
	static Engine *GetInstance();

	void init();
	void start();

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

	VkPipeline getGraphicsPipeline()
	{
		return graphicsPipeline;
	}

	VkPipelineLayout getPipelineLayout()
	{
		return pipelineLayout;
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

	void addSprite(Sprite sprite)
	{
		sprites.push_back(sprite);
	}

	void createGraphicsPipeline();
	void createCommandPool();

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer buffer);

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	void createCommandBuffers();
	void createSyncObjects();

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

	VkDeviceMemory &getUniformBufferMemory(uint32_t index)
	{
		return uniformBuffersMemory[index];
	}

	void setDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
	{
		this->descriptorSetLayout = descriptorSetLayout;
	}

	void createUniformBuffers();

	VkDescriptorSetLayout getDescriptorSetLayout() const
	{
		return descriptorSetLayout;
	}

	std::vector<Sprite> &getSprites()
	{
		return sprites;
	}

private:
	static const int MAX_FRAMES_IN_FLIGHT = 2;

	static Engine *pinstance_;
	static std::mutex mutex_;

	std::vector<Sprite> sprites;

	GLFWwindow *window;

	VkSurfaceKHR surface;

	VkInstance instance;

	SwapChain swapChain;

	VkDevice device;
	VkPhysicalDevice physicalDevice;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkRenderPass renderPass;

	MapRenderer *mapRenderer;

	VkDescriptorSetLayout descriptorSetLayout;

	VkDebugUtilsMessengerEXT debugMessenger;

	std::vector<VkCommandBuffer> commandBuffers;

	bool framebufferResized = false;

	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	std::vector<VkDeviceMemory> uniformBuffersMemory;

	void initWindow();
	void mainLoop();

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
};
