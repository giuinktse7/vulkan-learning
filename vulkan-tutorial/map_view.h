#pragma once

#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>

#include "map.h"
#include "camera.h"
#include "position.h"

#include "action/action.h"

struct Viewport
{
	struct BoundingRect
	{
		int x1, y1, x2, y2;
	};

	int width;
	int height;
	float zoom;

	uint32_t offsetX;
	uint32_t offsetY;
};

class MapView
{
public:
	MapView(GLFWwindow *window);

	std::optional<Position> moveSelectionOrigin;

	Map *getMap() const
	{
		return map.get();
	}

	bool isSelectionMoved() const
	{
		return moveSelectionOrigin.has_value();
	}

	void zoomOut();
	void zoomIn();
	void resetZoom();

	void updateViewport();

	float getZoomFactor() const;

	const MapPosition worldToMapPos(WorldPosition worldPos) const;
	const Position screenToMapPos(ScreenPosition screenPos) const;
	const MapPosition windowToMapPos(ScreenPosition screenPos) const;
	const uint32_t windowToMapPos(int windowPos) const;
	const uint32_t mapToWorldPos(uint32_t mapPos) const;

	uint32_t getZ() const
	{
		return static_cast<uint32_t>(camera.position.z);
	}

	/*
		Synonym for getZ()
	*/
	uint32_t getFloor() const
	{
		return getZ();
	}

	uint32_t getX() const
	{
		return static_cast<uint32_t>(camera.position.x);
	}
	uint32_t getY() const
	{
		return static_cast<uint32_t>(camera.position.y);
	}

	void translateCamera(glm::vec3 delta);

	void translateCameraZ(int z);

	const Viewport &getViewport() const
	{
		return viewport;
	}

	Viewport::BoundingRect getGameBoundingRect() const;

private:
	GLFWwindow *window;
	Viewport viewport;

	Camera camera;

	std::shared_ptr<Map> map;
};

inline std::ostream &operator<<(std::ostream &os, const Viewport::BoundingRect &rect)
{
	os << "{ x1=" << rect.x1 << ", y1=" << rect.y1 << ", x2=" << rect.x2 << ", y2=" << rect.y2 << "}";
	return os;
}