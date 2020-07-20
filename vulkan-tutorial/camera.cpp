#include "camera.h"

#include "graphics/engine.h"
#include "Logger.h"

void Camera::updateZoom()
{
	glm::vec2 mousePos = g_engine->getMousePosition();
	float n = 0.1f;
	float zoomFactor = n * exp(log(1 / n) / 10 * zoomStep);

	this->position.x -= mousePos.x / this->zoomFactor;
	this->position.x += mousePos.x / zoomFactor;

	this->position.y -= mousePos.y / this->zoomFactor;
	this->position.y += mousePos.y / zoomFactor;

	this->zoomFactor = zoomFactor;
}