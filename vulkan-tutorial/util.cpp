#define GLFW_INCLUDE_VULKAN

#include "util.h"
#include <GLFW/glfw3.h>
#include "graphics/validation.h"

#include <algorithm>

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

void to_lower_str(std::string &source)
{
  std::transform(source.begin(), source.end(), source.begin(), tolower);
}

void to_upper_str(std::string &source)
{
  std::transform(source.begin(), source.end(), source.begin(), toupper);
}

std::string as_lower_str(std::string s)
{
  to_lower_str(s);
  return s;
}

TimeMeasure TimeMeasure::start()
{
  return TimeMeasure();
}

TimeMeasure::TimeMeasure()
{
  startTime = std::chrono::high_resolution_clock::now();
}

long long TimeMeasure::elapsedMillis()
{
  auto stopTime = std::chrono::high_resolution_clock::now();
  long long durationMillis = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();

  return durationMillis;
}