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
	g_engine->setFrameBufferResized(true);
	g_engine->nextFrame();
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (!g_engine->captureKeyboard)
		return;
	bool ctrlDown = g_engine->isCtrlDown();

	// Handle CTRL
	if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
	{
		if (action == GLFW_RELEASE)
		{
			g_engine->setKeyUp(key);
		}
		else if (action == GLFW_PRESS)
		{
			g_engine->setKeyDown(key);
		}
	}

	bool keyActive = action == GLFW_PRESS || action == GLFW_REPEAT;

	if (!keyActive)
	{
		return;
	}

	if (ctrlDown && key == GLFW_KEY_0)
	{
		g_engine->resetZoom();
	}

	glm::vec2 delta(0.0f, 0.0f);
	float step = Engine::TILE_SIZE / std::pow(g_engine->getMapRenderer()->camera.zoomFactor);

	if (key == GLFW_KEY_RIGHT)
	{
		delta.x = step;
	}
	else if (key == GLFW_KEY_LEFT)
	{
		delta.x = -step;
	}
	else if (key == GLFW_KEY_UP)
	{
		delta.y = -step;
	}
	else if (key == GLFW_KEY_DOWN)
	{
		delta.y = step;
	}

	g_engine->translateCamera(delta);
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	if (!g_engine->captureMouse)
		return;
	if (yoffset > 0)
	{
		g_engine->zoomIn();
	}
	else
	{
		g_engine->zoomOut();
	}
}

static void cursorPositionCallback(GLFWwindow *window, double x, double y)
{
	if (!g_engine->captureMouse)
		return;
	Position oldGamePos = g_engine->screenToGamePos(g_engine->getMousePosition());

	g_engine->setMousePosition((float)x, (float)y);
	Position newGamePos = g_engine->screenToGamePos(g_engine->getMousePosition());
	if (oldGamePos != newGamePos)
	{
		// Dragging
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			auto &map = *g_engine->getMapRenderer()->map;
			auto pos = g_engine->screenToGamePos(g_engine->getMousePosition());

			auto item = Item::create(g_engine->getSelectedServerId());
			map.getOrCreateTile(pos).addItem(std::move(item));
		}
	}
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
	if (!g_engine->captureMouse)
		return;

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
	}
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
	glfwSetMouseButtonCallback(window, mouseButtonCallback);

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
	auto &map = *g_engine->getMapRenderer()->map;
	Tile &tile1 = map.getOrCreateTile(17, 1, 7);
	// Tile &tile2 = map.getOrCreateTile(1, 1, 7);
	// Tile &tile3 = map.getOrCreateTile(3, 1, 7);
	// Tile &tile4 = map.getOrCreateTile(4, 1, 7);
	// Tile &tile4 = map.createTile(4, 4, 7);

	// // auto shovel = Item::create(2554);
	auto shovel = Item::create(2554);
	auto tree = Item::create(2706);
	auto grass = Item::create(103);

	tile1.addItem(std::move(shovel));

	for (const auto &tile : map.begin())
	{
		std::cout << tile->getPosition() << std::endl;
	}
}

class X
{
public:
	X()
	{
		std::cout << "..............Construct X.............." << std::endl;
	}

	~X()
	{
		std::cout << "..............Destruct X.............." << std::endl;
	}
};

X x;

int main()
{
	MapRenderer *mapRenderer;

	try
	{
		engine::create();

		Appearances::loadFromFile("data/appearances.dat");
		std::cout << "Loaded appearances.dat." << std::endl;
		Appearances::loadCatalog("data/catalog-content.json");
		std::cout << "Loaded catalog-content.json." << std::endl;

		Items::loadFromOtb("data/items.otb");
		Items::loadFromXml("data/items.xml");

		mapRenderer = new MapRenderer(std::make_unique<Map>());
		g_engine->setMapRenderer(mapRenderer);

		// populateTestMap();

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