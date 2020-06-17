#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
private:
  float fov;
  float zNear = -1.0f;
  float zfar = 1.0f;

public:
  glm::vec2 position = glm::vec2();
  glm::vec4 viewPos = glm::vec4();

  float zoomFactor = 1.0f;

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

  float getNearClip()
  {
    return zNear;
  }

  float getFarClip()
  {
    return zfar;
  }

  void setZoom(float zoomFactor)
  {
    this->zoomFactor = zoomFactor;
    matrices.perspective = glm::ortho(
        -1.0f / zoomFactor,
        1.0f / zoomFactor,
        -1.0f / zoomFactor,
        1.0f / zoomFactor,
        zNear,
        zfar);
  }

  void updateProjection()
  {
    matrices.perspective = glm::ortho(
        -1.0f / zoomFactor,
        1.0f / zoomFactor,
        -1.0f / zoomFactor,
        1.0f / zoomFactor,
        zNear,
        zfar);
  };

  void updateAspectRatio(float aspect)
  {
    matrices.perspective = glm::perspective(glm::radians(fov), aspect, zNear, zfar);
  }

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

  void zoomIn()
  {
    this->zoomFactor *= 1.2;
    updateProjection();
  }

  void zoomOut()
  {
    this->zoomFactor /= 1.2f;
    updateProjection();
  }

  void resetZoom()
  {
    this->zoomFactor = 1.0f;
    updateProjection();
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