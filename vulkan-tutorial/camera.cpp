#include "camera.h"

#include "graphics/engine.h"
#include "Logger.h"

void Camera::updateZoom()
{
	glm::vec2 mousePos = g_engine->getMousePosition();
	float n = 0.1f;
	float zoomFactor = n * exp(log(1 / n) / 10 * zoomStep);

	glm::vec2 newPos{this->position};

	newPos.x += mousePos.x / this->zoomFactor;
	newPos.x -= mousePos.x / zoomFactor;

	newPos.y += mousePos.y / this->zoomFactor;
	newPos.y -= mousePos.y / zoomFactor;

	this->setPosition(newPos);

	this->zoomFactor = zoomFactor;
}

bool Camera::isMoving()
{
	return keys.left || keys.right || keys.up || keys.down;
}

void Camera::setPosition(glm::vec2 position)
{
	this->position = glm::vec2(std::max(position.x, 0.0f), std::max(position.y, 0.0f));
}

void Camera::translate(glm::vec2 delta)
{
	setPosition(this->position + delta);
}

void Camera::zoomIn()
{
	zoomStep = std::clamp(zoomStep + 1, 0, zoomSteps);
	updateZoom();
}

void Camera::zoomOut()
{
	zoomStep = std::clamp(zoomStep - 1, 0, zoomSteps);
	updateZoom();
}

void Camera::resetZoom()
{
	zoomStep = 10;
	updateZoom();
}

void Camera::setMovementSpeed(float movementSpeed)
{
	this->movementSpeed = movementSpeed;
}

void Camera::update(float deltaTime)
{
	updated = false;
	if (isMoving())
	{
		float moveSpeed = deltaTime * movementSpeed;

		if (keys.up)
			position += moveSpeed;
		if (keys.down)
			position -= moveSpeed;
		if (keys.left)
			position -= moveSpeed;
		if (keys.right)
			position += moveSpeed;
	}
};