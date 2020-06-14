#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "sprite.h"
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
	Engine *engine = Engine::GetInstance();
	Logger::info("Changed");
	engine->setFrameBufferResized(true);

	if (engine->StartFrame())
	{
		engine->SetTexture(crossbowTexture);
		engine->DrawSprite(32, 32, crossbowTexture->getWidth(), crossbowTexture->getHeight());

		engine->SetTexture(plateArmorTexture);
		engine->DrawSprite(64, 32, plateArmorTexture->getWidth(), plateArmorTexture->getHeight());
		engine->EndFrame();
	}
}

GLFWwindow *initWindow()
{
	GLFWwindow *window;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	// glfwSetWindowUserPointer(window, this);

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
			if (engine.StartFrame())
			{
				engine.SetTexture(crossbowTexture);
				engine.DrawSprite(32, 32, crossbowTexture->getWidth(), crossbowTexture->getHeight());

				engine.SetTexture(plateArmorTexture);
				engine.DrawSprite(64, 32, plateArmorTexture->getWidth(), plateArmorTexture->getHeight());
				engine.EndFrame();
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