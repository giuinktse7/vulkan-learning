#include "camera.h"

#include "graphics/engine.h"
#include "Logger.h"

void Camera::updateZoom()
{
	glm::vec2 mousePos = g_engine->getMousePosition();
	float n = 0.1f;
	float zoomFactor = n * exp(log(1 / n) / 10 * zoomStep);

	glm::vec3 newPos{this->position};

	newPos.x += mousePos.x / this->zoomFactor;
	newPos.x -= mousePos.x / zoomFactor;

	newPos.y += mousePos.y / this->zoomFactor;
	newPos.y -= mousePos.y / zoomFactor;

	this->setPosition(newPos);

	this->zoomFactor = zoomFactor;
}

void Camera::setPosition(glm::vec3 position)
{
	this->position = glm::vec3(std::max(position.x, 0.0f), std::max(position.y, 0.0f), std::clamp(static_cast<int>(position.z), 0, 15));
}

void Camera::translate(glm::vec3 delta)
{
	setPosition(this->position + delta);
}

void Camera::translateZ(int z)
{
	this->position.z = std::clamp(static_cast<int>(position.z + z), 0, 15);
}

void Camera::zoomIn()
{
	setZoomStep(this->zoomStep + 1);
}

void Camera::zoomOut()
{
	setZoomStep(this->zoomStep - 1);
}

void Camera::resetZoom()
{
	setZoomStep(10);
}

void Camera::setZoomStep(int zoomStep)
{
	if (this->zoomStep != zoomStep)
	{
		this->zoomStep = std::clamp(zoomStep, 0, zoomSteps);
		updateZoom();
	}
}