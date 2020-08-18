#pragma once

#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <set>

#include "map.h"
#include "camera.h"
#include "position.h"
#include "util.h"

#include "action/action.h"

struct Viewport
{

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
	EditorHistory history;

	Map *getMap() const
	{
		return map.get();
	}

	void addItem(const Position position, uint16_t id);

	/* Note: The indices must be in descending order (std::greater), because
		otherwise the wrong items could be removed.
	*/
	void removeItems(const Position position, const std::set<size_t, std::greater<size_t>> &indices);
	void removeGround(const Position position);
	void removeTile(const Position position);

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

	void undo()
	{
		history.undoLast();
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

	util::Rectangle<int> getGameBoundingRect() const;

	void setDragStart(WorldPosition position);
	void setDragEnd(WorldPosition position);
	std::optional<std::pair<WorldPosition, WorldPosition>> getDragPoints() const
	{
		if (dragState.has_value())
		{
			std::pair<WorldPosition, WorldPosition> result;
			result.first = dragState.value().from;
			result.second = dragState.value().to;
			return result;
		}

		return {};
	}
	void endDragging();
	bool isDragging() const;

private:
	GLFWwindow *window;
	Viewport viewport;

	struct DragData
	{
		WorldPosition from, to;
	};
	std::optional<DragData> dragState;

	Camera camera;

	std::shared_ptr<Map> map;

	Tile deepCopyTile(const Position position) const
	{
		return map->getTile(position)->deepCopy();
	}
};

inline std::ostream &operator<<(std::ostream &os, const util::Rectangle<int> &rect)
{
	os << "{ x1=" << rect.x1 << ", y1=" << rect.y1 << ", x2=" << rect.x2 << ", y2=" << rect.y2 << "}";
	return os;
}