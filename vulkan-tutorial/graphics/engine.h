#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <imgui_impl_vulkan.h>

#include <unordered_set>
#include <mutex>

#include "../position.h"

#include "device_manager.h"
#include "../map_renderer.h"
#include "texture.h"
#include "resource-descriptor.h"
#include "buffer.h"
#include "vertex.h"
#include "../camera.h"

#include "../map.h"

#include "../gui/gui.h"

class Engine;

extern Engine *g_engine;

namespace engine
{

	void create();

} // namespace engine

class Engine
{
public:
	Engine();
	~Engine();

	unsigned long frames = 0;

	static const int TILE_SIZE = 32;

	/* Vulkan helpers */
	VkResult mapMemory(
			VkDeviceMemory memory,
			VkDeviceSize offset,
			VkDeviceSize size,
			VkMemoryMapFlags flags,
			void **ppData);

	VkInstance &getVkInstance()
	{
		return instance;
	}

	void setPhysicalDevice(VkPhysicalDevice physicalDevice)
	{
		this->physicalDevice = physicalDevice;
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

	VkPhysicalDevice &getPhysicalDevice()
	{
		return physicalDevice;
	}

	VkQueue *getGraphicsQueue()
	{
		return &graphicsQueue;
	}

	VkQueue *getPresentQueue()
	{
		return &presentQueue;
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

	VkCommandPool getCommandPool()
	{
		return commandPool;
	}

	void clearCurrentCommandBuffer()
	{
		currentCommandBuffer = nullptr;
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

	void shutdown();

	void WaitUntilDeviceIdle();

	QueueFamilyIndices getQueueFamilyIndices()
	{
		return queueFamilyIndices;
	}

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

	void endFrame();

	bool isValidWindowSize();

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

	void setMousePosition(float x, float y)
	{
		this->mousePosition.x = x;
		this->mousePosition.y = y;
	}

	glm::vec2 getMousePosition() const
	{
		return mousePosition;
	}

	VkAllocationCallbacks *getAllocator()
	{
		return allocator;
	}

	uint32_t getImageCount()
	{
		return swapChain.getImageCount();
	}

	uint32_t getMinImageCount()
	{
		return swapChain.getMinImageCount();
	}

	uint32_t getWidth()
	{
		return swapChain.getExtent().width;
	}
	uint32_t getHeight()
	{
		return swapChain.getExtent().height;
	}

	bool initFrame();
	void nextFrame();

	VkShaderModule createShaderModule(const std::vector<uint8_t> &code);

	void resetZoom()
	{
		mapRenderer->camera.resetZoom();
	}

	void zoomIn()
	{
		mapRenderer->camera.zoomIn();
	}

	void zoomOut()
	{
		mapRenderer->camera.zoomOut();
	}

	int getCameraZoomStep()
	{
		return mapRenderer->camera.zoomStep;
	}

	void translateCamera(glm::vec2 delta)
	{
		return mapRenderer->camera.translate(delta);
	}

	VkDescriptorPool &getMapDescriptorPool()
	{
		return mapRenderer->getDescriptorPool();
	}

	VkDescriptorSetLayout &getTextureDescriptorSetLayout()
	{
		return mapRenderer->getTextureDescriptorSetLayout();
	}

	MapRenderer *getMapRenderer()
	{
		return mapRenderer;
	}

	const glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

private:
	glm::vec2 mousePosition;
	bool isInitialized = false;

	GUI gui;

	static Engine *pinstance_;
	static std::mutex mutex_;

	GLFWwindow *window;

	int width;
	int height;

	VkSurfaceKHR surface;

	VkInstance instance;
	VkAllocationCallbacks *allocator = NULL;

	SwapChain swapChain;

	VkDevice device;
	VkPhysicalDevice physicalDevice;

	QueueFamilyIndices queueFamilyIndices;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	MapRenderer *mapRenderer;

	VkDebugUtilsMessengerEXT debugMessenger;

	bool framebufferResized = false;

	VkCommandPool commandPool;

	std::vector<BoundBuffer> uniformBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	VkDescriptorSetLayout perTextureDescriptorSetLayout;
	VkDescriptorSetLayout perFrameDescriptorSetLayout;

	int currentBufferIndex = 0;

	VkCommandBuffer currentCommandBuffer;

	uint32_t previousFrame;
	uint32_t currentFrame;

	std::unordered_set<int> keys;

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

	void presentFrame();
	void recreateSwapChain();
};