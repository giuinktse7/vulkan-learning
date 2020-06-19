#pragma once

#include <vector>
#include <optional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete()
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

namespace VulkanHelpers
{
  static const char *khronosValidation = "VK_LAYER_KHRONOS_validation";
  static const char *standardValidation = "VK_LAYER_LUNARG_standard_validation";

  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

  VkImageView createImageView(VkDevice device, VkImage image, VkFormat format);

  VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);

  uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

  void createCommandPool(VkCommandPool *commandPool, VkCommandPoolCreateFlags flags);
  void createCommandBuffers(VkCommandBuffer *commandBuffer, uint32_t commandBufferCount, VkCommandPool &commandPool);

} // namespace VulkanHelpers