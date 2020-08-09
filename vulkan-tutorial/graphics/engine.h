#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../random.h"

#include "../gui/imgui_impl_vulkan.h"

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

#include "../util.h"
#include "../time.h"

enum class FrameResult
{
	Failure = 0,
	Success = 1
};

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

	static const int TILE_SIZE = 32;
	const glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	TimePoint startTime;

	/*
		Used by animators for synchronous animations
	*/
	TimePoint currentTime;

	uint32_t currentFrameIndex;

	Random random;

	bool debug = false;

	// Can be set to false if the UI is currently capturing the mouse.
	bool captureMouse = true;
	bool captureKeyboard = true;

	struct FrameData
	{
		VkSemaphore imageAvailableSemaphore = nullptr;
		VkSemaphore renderCompleteSemaphore = nullptr;
		VkFence inFlightFence = nullptr;
	};

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

	const uint32_t gameToWorldPos(uint32_t gamePos) const;
	const uint32_t worldToGamePos(float worldPos) const;
	const Position screenToGamePos(float screenX, float screenY) const;
	const uint32_t screenToGamePos(float value) const;
	const Position screenToGamePos(glm::vec2 pos) const;

	void createCommandPool();

	void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer buffer);

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

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

	uint32_t getMaxFramesInFlight();

	bool isValidWindowSize();

	void setKeyState(int key, int state);
	int getKeyState(int key);

	void setMousePosition(float x, float y)
	{
		this->prevMousePosition = this->mousePosition;
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
	FrameResult nextFrame();

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

	void translateCamera(glm::vec3 delta);
	void translateCameraZ(int z);

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

	const std::optional<uint16_t> getSelectedServerId() const
	{
		return gui.brushServerId;
	}

	/*
	Remove selected item from the brush
	*/
	void clearBrush()
	{
		gui.brushServerId.reset();
	}

private:
	std::array<FrameData, 3> frames;
	// Fences for vkAcquireNextImageKHR
	std::array<VkFence, 3> swapChainImageInFlight;
	FrameData *currentFrame = nullptr;

	glm::vec2 prevMousePosition;
	glm::vec2 mousePosition;
	bool isInitialized = false;

	GUI gui;
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

	VkDescriptorSetLayout perTextureDescriptorSetLayout;
	VkDescriptorSetLayout perFrameDescriptorSetLayout;

	VkCommandBuffer currentCommandBuffer;

	uint32_t previousFrame;

	std::map<int, int> keyState;

	void createVulkanInstance();

	static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

	static bool checkValidationLayerSupport();
	static std::vector<const char *> getRequiredExtensions();
	static bool chronosOrStandardValidation(std::vector<VkLayerProperties> &props);
	void createSurface();

	void setFrameIndex(uint32_t index);

	void setSurface(VkSurfaceKHR &surface)
	{
		this->surface = surface;
	}

	void setWindow(GLFWwindow *window)
	{
		this->window = window;
	}

	void recreateSwapChain();
};