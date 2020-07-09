#include "camera.h"

#include "graphics/engine.h"
#include "Logger.h"

void Camera::updateZoom()
{
	glm::vec2 mousePos = Engine::getInstance()->getMousePosition();
	float n = 0.1;
	float zoomFactor = n * exp(log(1 / n) / 10 * zoomStep);

	this->position.x -= mousePos.x / this->zoomFactor;
	this->position.x += mousePos.x / zoomFactor;

	this->position.y -= mousePos.y / this->zoomFactor;
	this->position.y += mousePos.y / zoomFactor;

	this->zoomFactor = zoomFactor;
}