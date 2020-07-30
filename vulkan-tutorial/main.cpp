#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <algorithm>
#include <optional>
#include <set>
#include <fstream>
#include <array>
#include <cassert>

#include "input.h"

#include "map_io.h"

#include "quad_tree.h"
#include "tile_location.h"

#include "debug.h"

#include "util.h"
#include "file.h"

#include "graphics/engine.h"

#include "Logger.h"

#include "graphics/vulkan_helpers.h"
#include "graphics/resource-descriptor.h"

#include "graphics/texture.h"

#include "graphics/appearances.h"
#include "map.h"

using namespace std;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

void framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
	g_engine->setFrameBufferResized(true);
	g_engine->nextFrame();
}

GLFWwindow *initWindow()
{
	GLFWwindow *window;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan map editor", nullptr, nullptr);
	glfwSetKeyCallback(window, Input::handleKeyAction);
	glfwSetScrollCallback(window, Input::handleMouseScroll);
	glfwSetCursorPosCallback(window, Input::handleCursorPosition);
	glfwSetMouseButtonCallback(window, Input::handleMouseKeyAction);

	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	return window;
}

void populateTestMap()
{
	auto &map = *g_engine->getMapRenderer()->map;
	Tile &tile1 = map.getOrCreateTile(17, 1, 7);
	// Tile &tile2 = map.getOrCreateTile(1, 1, 7);
	// Tile &tile3 = map.getOrCreateTile(3, 1, 7);
	// Tile &tile4 = map.getOrCreateTile(4, 1, 7);
	// Tile &tile4 = map.createTile(4, 4, 7);

	// // auto shovel = Item::create(2554);
	auto shovel = Item::create(2554);
	// auto tree = Item::create(2706);
	// auto grass = Item::create(103);

	tile1.addItem(std::move(shovel));

	// for (const auto &tile : map.begin())
	// {
	// 	std::cout << tile->getPosition() << std::endl;
	// }
}

int main()
{
	MapRenderer *mapRenderer;

	std::cout << "Saving.. " << std::endl;

	try
	{
		engine::create();

		Appearances::loadFromFile("data/appearances.dat");
		Appearances::loadCatalog("data/catalog-content.json");
		std::cout << "Loaded catalog-content.json." << std::endl;

		Items::loadFromOtb("data/items.otb");
		Items::loadFromXml("data/items.xml");

		mapRenderer = new MapRenderer(std::make_unique<Map>());
		g_engine->setMapRenderer(mapRenderer);

		// populateTestMap();
		// MapIO::saveMap(*mapRenderer->map);

		// return 0;

		GLFWwindow *window = initWindow();
		g_engine->initialize(window);

		g_engine->getMapRenderer()->loadTextureAtlases();

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			g_engine->nextFrame();
		}

		g_engine->WaitUntilDeviceIdle();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}