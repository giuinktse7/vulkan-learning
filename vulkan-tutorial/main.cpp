#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

#include <stdlib.h>

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
#include "ecs/item_animation.h"
#include "ecs/item_selection.h"
#include "map.h"
#include "ecs/ecs.h"

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

class X
{
};

class Y
{
};

constexpr int IdleFrameTime = 1000 / 60;

int main()
{
	MapRenderer *mapRenderer;

	try
	{
		engine::create();

		// Register animation
		g_ecs.registerComponent<ItemAnimationComponent>();
		g_ecs.registerSystem<ItemAnimationSystem>();

		// Register selection
		g_ecs.registerComponent<TileSelectionComponent>();
		g_ecs.registerSystem<TileSelectionSystem>();

		Appearances::loadTextureAtlases("data/catalog-content.json");
		Appearances::loadAppearanceData("data/appearances.dat");

		Items::loadFromOtb("data/items.otb");
		Items::loadFromXml("data/items.xml");

		mapRenderer = new MapRenderer(std::make_unique<Map>());
		g_engine->setMapRenderer(mapRenderer);

		// populateTestMap();
		// MapIO::saveMap(*mapRenderer->map);

		GLFWwindow *window = initWindow();
		g_engine->initialize(window);
		Logger::info() << "Loading finished in " << g_engine->startTime.elapsedMillis() << " ms." << std::endl;

		long long x = 0;

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			Input::update(window);
			FrameResult res = g_engine->nextFrame();
			if (res == FrameResult::Success)
			{
				g_ecs.getSystem<ItemAnimationSystem>().update();
			}
			else
			{
				++x;
				std::this_thread::sleep_for(std::chrono::milliseconds(IdleFrameTime));
				if (x % 60 == 0)
				{
					std::cout << "Failed count: " << x << std::endl;
				}
			}
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