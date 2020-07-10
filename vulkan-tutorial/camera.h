#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
  glm::vec4 viewPos = glm::vec4();

  float zoomFactor = 1.0f;
  int zoomStep = 15;

  float movementSpeed = 1.0f;
  bool updated = false;

  struct
  {
    glm::mat4 perspective;
    glm::mat4 view;
  } matrices;

  struct
  {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
  } keys;

  bool moving()
  {
    return keys.left || keys.right || keys.up || keys.down;
  }

  void updateProjection(){};

  void setPosition(glm::vec2 position)
  {
    this->position = position;
  }

  void setTranslation(glm::vec2 translation)
  {
    this->position = translation;
  };

  void translate(glm::vec2 delta)
  {
    this->position += delta;
  }

  void updateZoom();

  void zoomIn()
  {
    zoomStep = std::clamp(zoomStep + 1, 0, zoomSteps);
    updateZoom();
  }

  void zoomOut()
  {
    zoomStep = std::clamp(zoomStep - 1, 0, zoomSteps);
    updateZoom();
  }

  void resetZoom()
  {
    zoomStep = 10;
    updateZoom();
  }

  void setMovementSpeed(float movementSpeed)
  {
    this->movementSpeed = movementSpeed;
  }

  void update(float deltaTime)
  {
    updated = false;
    if (moving())
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
};