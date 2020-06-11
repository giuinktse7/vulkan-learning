#define GLFW_INCLUDE_VULKAN

#include "util.h"
#include <GLFW/glfw3.h>
#include "validation.h"

std::vector<const char *> getRequiredExtensions()
{
  uint32_t glfwExtensionCount = 0;

  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (Validation::enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}
