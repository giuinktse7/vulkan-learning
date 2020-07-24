#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

class Camera
{
private:
  int zoomSteps = 20;

public:
  glm::vec2 position = glm::vec2();

  float zoomFactor = 1.0f;
  int zoomStep = 10;

  float movementSpeed = 1.0f;
  bool updated = false;

  struct
  {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
  } keys;

  bool isMoving();

  void setPosition(glm::vec2 position);

  void translate(glm::vec2 delta);

  void updateZoom();

  void zoomIn();

  void zoomOut();

  void resetZoom();

  void setMovementSpeed(float movementSpeed);

  void update(float deltaTime);
};