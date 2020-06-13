#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>

namespace DeviceManager
{

  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  void createLogicalDevice();
}; // namespace DeviceManager