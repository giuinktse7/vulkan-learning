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
#include "map.h"
#include "ecs/ecs.h"
#include "input_control.h"

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

	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	return window;
}

void populateTestMap()
{
	// auto &map = *g_engine->getMapRenderer()->getMap();
	// Tile &tile1 = map.getOrCreateTile(17, 1, 7);
	// Tile &tile2 = map.getOrCreateTile(1, 1, 7);
	// Tile &tile3 = map.getOrCreateTile(3, 1, 7);
	// Tile &tile4 = map.getOrCreateTile(4, 1, 7);
	// Tile &tile4 = map.createTile(4, 4, 7);

	// // auto shovel = Item::create(2554);
	// auto shovel = Item::create(2554);
	// auto tree = Item::create(2706);
	// auto grass = Item::create(103);

	// tile1.addItem(std::move(shovel));

	// for (const auto &tile : map.begin())
	// {
	// 	std::cout << tile->getPosition() << std::endl;
	// }
}

#include <functional>

constexpr int IdleFrameTime = 1000 / 60;

#include "type_trait.h"

class ItemMock
{
public:
	ItemMock(std::string name) : name(name) {}
	~ItemMock() { std::cout << "~ItemMock(" << name << ")" << std::endl; }
	ItemMock(ItemMock &&other) : name(std::move(other.name))
	{
	}

	std::string name;
};

class TileLocationMock
{
public:
	TileLocationMock(std::string s) : s(s) {}
	TileLocationMock(const TileLocationMock &) = delete;
	TileLocationMock &operator=(const TileLocationMock &) = delete;

	std::string s;
};

class TileMock
{
public:
	TileMock(TileLocationMock &location) : location(location) {}
	TileMock(const TileMock &) = delete;
	TileMock(TileMock &&other) : location(other.location)
	{
		items = std::move(other.items);
		std::cout << "After" << std::endl;
	};
	TileMock &operator=(const TileMock &) = delete;

	TileLocationMock &location;
	std::vector<ItemMock> items;

	TileMock deepCopy()
	{
		TileMock copy = TileMock(this->location);
		return copy;
	}
};

class ChangeMock
{
public:
	ChangeMock(TileMock &&tile) : data(std::move(tile)) {}
	std::variant<TileMock> data;
};

void testy()
{
	TileLocationMock loc("test");
	TileMock tile(loc);
	tile.items.emplace_back("what");
	tile.items.emplace_back("is");
	tile.items.emplace_back("this");

	ChangeMock change(std::move(tile));

	std::cout << std::get<TileMock>(change.data).location.s << std::endl;
}

class K
{
};

int main()
{
	try
	{
		// testy();
		// return 0;

		engine::create();

		// Register animation
		g_ecs.registerComponent<ItemAnimationComponent>();
		g_ecs.registerSystem<ItemAnimationSystem>();

		Appearances::loadTextureAtlases("data/catalog-content.json");
		Appearances::loadAppearanceData("data/appearances.dat");

		Items::loadFromOtb("data/items.otb");
		Items::loadFromXml("data/items.xml");

		// populateTestMap();
		// MapIO::saveMap(*mapRenderer->map);

		GLFWwindow *window = initWindow();
		Input input(window);
		input.registerHook(InputControl::cameraMovement);
		input.registerHook(InputControl::mapEditing);

		g_engine->initialize(window);
		Logger::info() << "Loading finished in " << g_engine->startTime.elapsedMillis() << " ms." << std::endl;

		bool captureMouse = g_engine->captureMouse;

		while (!glfwWindowShouldClose(window))
		{
			g_engine->gui.captureIO();
			glfwPollEvents();

			if (captureMouse != g_engine->captureMouse)
			{
				Logger::debug() << "captureMouse changed to " << g_engine->captureMouse << std::endl;
			}

			captureMouse = g_engine->captureMouse;

			FrameResult res = g_engine->nextFrame();
			if (res == FrameResult::Success)
			{
				input.update();
				g_ecs.getSystem<ItemAnimationSystem>().update();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(IdleFrameTime));
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