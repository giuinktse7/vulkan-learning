#include "map_view.h"

#include "const.h"

MapView::MapView(GLFWwindow *window) : window(window), map(std::make_shared<Map>())
	,moveSelectionOrigin{}
{
}

void MapView::updateViewport()
{
  glfwGetFramebufferSize(window, &viewport.width, &viewport.height);

  viewport.zoom = 1 / camera.zoomFactor;
  viewport.offsetX = camera.position.x;
  viewport.offsetY = camera.position.y;
}

void MapView::zoomOut()
{
  camera.zoomOut();
}
void MapView::zoomIn()
{
  camera.zoomIn();
}
void MapView::resetZoom()
{
  camera.resetZoom();
}

float MapView::getZoomFactor() const
{
  return camera.zoomFactor;
}

void MapView::translateCamera(glm::vec3 delta)
{
  camera.translate(delta);
}
void MapView::translateCameraZ(int z)
{
  camera.translateZ(z);
}

Viewport::BoundingRect MapView::getGameBoundingRect() const
{
  WorldPosition worldPos{viewport.offsetX, viewport.offsetY};
  MapPosition mapPos = worldPos.mapPos();

  auto [width, height] = ScreenPosition{static_cast<double>(viewport.width), static_cast<double>(viewport.height)}.mapPos(*this);
  Viewport::BoundingRect rect;
  rect.x1 = mapPos.x;
  rect.y1 = mapPos.y;
  rect.x2 = mapPos.x + width;
  rect.y2 = mapPos.y + height;

  return rect;
}