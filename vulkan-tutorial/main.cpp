#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "pipeline.h"

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

#include "engine.h"

#include "Logger.h"

#include "VulkanHelpers.h"

#include "resource-descriptor.h"

#include "vertex.h"
#include "texture.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

std::shared_ptr<Texture> crossbowTexture;
std::shared_ptr<Texture> plateArmorTexture;

void framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
	Engine *engine = Engine::getInstance();
	engine->setFrameBufferResized(true);

	if (engine->startFrame())
	{
		engine->setTexture(crossbowTexture);
		engine->drawSprite(32, 32, crossbowTexture->getWidth(), crossbowTexture->getHeight());

		engine->setTexture(plateArmorTexture);
		engine->drawSprite(64, 32, plateArmorTexture->getWidth(), plateArmorTexture->getHeight());
		engine->endFrame();
	}
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
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
		engine->camera.resetZoom();
	}

	glm::vec2 delta(0.0f, 0.0f);
	float step = 8.0f / std::min(engine->camera.zoomFactor, 1.0f);

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

	Engine::getInstance()->camera.translate(delta);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	Engine *engine = Engine::getInstance();
	if (yoffset > 0)
	{
		engine->camera.zoomIn();
	}
	else
	{
		engine->camera.zoomOut();
	}
}

GLFWwindow *initWindow()
{
	GLFWwindow *window;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Monko's map editor", nullptr, nullptr);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);

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

		while (!glfwWindowShouldClose(engine.getWindow()))
		{
			glfwPollEvents();
			if (engine.startFrame())
			{
				engine.setTexture(crossbowTexture);
				engine.drawSprite(32, 32, crossbowTexture->getWidth(), crossbowTexture->getHeight());

				engine.setTexture(plateArmorTexture);
				engine.drawSprite(64, 32, plateArmorTexture->getWidth(), plateArmorTexture->getHeight());
				engine.endFrame();
			}
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