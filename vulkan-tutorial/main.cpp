#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <algorithm>
#include <optional>
#include <set>
#include <fstream>
#include <array>

#include "validation.h"
#include "util.h"
#include "file.h"

#include "graphics/engine.h"

#include "Logger.h"

#include "graphics/vulkan_helpers.h"
#include "graphics/resource-descriptor.h"

#include "graphics/texture.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

std::shared_ptr<Texture> crossbowTexture;
std::shared_ptr<Texture> plateArmorTexture;
std::shared_ptr<Texture> blackMarbleFloor;

void drawScene()
{
	Engine *engine = Engine::getInstance();

	if (engine->initFrame())
	{
		engine->renderFrame();

		engine->setTexture(blackMarbleFloor);
		for (int x = 0; x < 50; ++x)
		{
			for (int y = 0; y < 50; ++y)
			{
				engine->drawSprite(x, y, crossbowTexture->getWidth(), crossbowTexture->getHeight());
			}
		}

		engine->setTexture(crossbowTexture);
		engine->drawSprite(3, 3, crossbowTexture->getWidth(), crossbowTexture->getHeight());

		engine->setTexture(plateArmorTexture);
		engine->drawSprite(4, 3, plateArmorTexture->getWidth(), plateArmorTexture->getHeight());
		engine->endFrame();
	}
}

void framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
	Engine *engine = Engine::getInstance();
	engine->setFrameBufferResized(true);

	drawScene();
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{

	Engine *engine = Engine::getInstance();

	bool ctrlDown = engine->isCtrlDown();

	// Handle CTRL
	if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
	{
		if (action == GLFW_RELEASE)
		{
			engine->setKeyUp(key);
		}
		else if (action == GLFW_PRESS)
		{
			engine->setKeyDown(key);
		}
	}

	bool keyActive = action == GLFW_PRESS || action == GLFW_REPEAT;

	if (!keyActive)
	{
		return;
	}

	if (ctrlDown && key == GLFW_KEY_0)
	{
		engine->resetZoom();
	}

	glm::vec2 delta(0.0f, 0.0f);
	float step = Engine::TILE_SIZE * 11 / (engine->getCameraZoomStep() + 1);

	if (key == GLFW_KEY_RIGHT)
	{
		delta.x = -step;
	}
	else if (key == GLFW_KEY_LEFT)
	{
		delta.x = step;
	}
	else if (key == GLFW_KEY_UP)
	{
		delta.y = step;
	}
	else if (key == GLFW_KEY_DOWN)
	{
		delta.y = -step;
	}

	Engine::getInstance()->translateCamera(delta);
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	Engine *engine = Engine::getInstance();
	if (yoffset > 0)
	{
		engine->zoomIn();
	}
	else
	{
		engine->zoomOut();
	}
}

static void cursorPositionCallback(GLFWwindow *window, double x, double y)
{
	Engine::getInstance()->setMousePosition(x, y);
}

GLFWwindow *initWindow()
{
	GLFWwindow *window;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Monko's map editor", nullptr, nullptr);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetCursorPosCallback(window, cursorPositionCallback);

	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	return window;
}

int main()
{
	MapRenderer *mapRenderer;

	Engine engine;

	try
	{
		Engine::setInstance(&engine);

		mapRenderer = new MapRenderer();
		engine.setMapRenderer(mapRenderer);

		GLFWwindow *window = initWindow();

		engine.initialize(window);

		crossbowTexture = engine.CreateTexture("textures/crossbow.png");
		plateArmorTexture = engine.CreateTexture("textures/plate_armor.png");
		blackMarbleFloor = engine.CreateTexture("textures/black_marble_floor.png");

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawScene();
		}

		engine.WaitUntilDeviceIdle();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}