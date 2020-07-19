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
	Engine *engine = Engine::getInstance();
	engine->setFrameBufferResized(true);

	engine->nextFrame();
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
	float step = static_cast<float>(Engine::TILE_SIZE) * 11 / (engine->getCameraZoomStep() + 1);

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
	Engine::getInstance()->setMousePosition((float)x, (float)y);
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

class TestX
{
public:
	TestX()
	{
		std::cout << "Constructed." << std::endl;
	}
	~TestX()
	{
		std::cout << "Destroyed." << std::endl;
	}

private:
	uint16_t x = 0;
};

void populateTestMap()
{
	auto engine = Engine::getInstance();
	auto &map = engine->map;
	Tile &tile1 = map.createTile(2, 1, 7);
	Tile &tile2 = map.createTile(2, 2, 7);
	Tile &tile3 = map.createTile(2, 3, 7);
	Tile& tile4 = map.createTile(4, 4, 7);

	// auto shovel = Item::create(2554);
	auto shovel = Item::create(2554);
	auto tree = Item::create(2706);
	auto grass = Item::create(103);
	auto waterBorder = Item::create(6643);
	auto border = Item::create(6565);
	auto border2 = Item::create(6565);
	auto border3 = Item::create(6565);

	tile4.addItem(std::move(tree));

	// tile1.addItem(std::move(border));
	tile2.addItem(std::move(border2));

	tile3.addItem(std::move(shovel));
	// tile.addItem(std::move(tree));
	tile3.addItem(std::move(grass));
	// tile3.addItem(std::move(waterBorder));
	tile3.addItem(std::move(border3));
}

int main()
{
	MapRenderer *mapRenderer;

	Engine engine;

	try
	{
		Appearances::loadFromFile("data/appearances.dat");
		std::cout << "Loaded appearances.dat." << std::endl;
		Appearances::loadCatalog("data/catalog-content.json");
		std::cout << "Loaded catalog-content.json." << std::endl;

		Items::loadFromOtb("data/items.otb");
		Items::loadFromXml("data/items.xml");

		Engine::setInstance(&engine);

		mapRenderer = new MapRenderer();
		engine.setMapRenderer(mapRenderer);

		populateTestMap();

		// return 0;

		GLFWwindow *window = initWindow();
		engine.initialize(window);

		engine.getMapRenderer()->loadTextureAtlases();

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			engine.nextFrame();
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