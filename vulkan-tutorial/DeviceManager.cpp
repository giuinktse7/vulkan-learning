#include "DeviceManager.h"

#include <vector>
#include <stdexcept>
#include <set>

#include "SwapChain.h"

#include "validation.h"
#include "VulkanHelpers.h"
#include "Engine.h"

void DeviceManager::pickPhysicalDevice()
{
  Engine *engine = Engine::GetInstance();
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(engine->getVkInstance(), &deviceCount, nullptr);

  if (deviceCount == 0)
  {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(engine->getVkInstance(), &deviceCount, devices.data());

  for (const auto &device : devices)
  {
    if (isDeviceSuitable(device))
    {
      engine->setPhysicalDevice(device);
      break;
    }
  }

  if (engine->getPhysicalDevice() == VK_NULL_HANDLE)
  {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

bool DeviceManager::isDeviceSuitable(VkPhysicalDevice device)
{
  Engine *engine = Engine::GetInstance();
  QueueFamilyIndices indices = VulkanHelpers::findQueueFamilies(device, engine->getSurface());

  bool extensionsSupported = VulkanHelpers::checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported)
  {
    SwapChainSupportDetails swapChainSupport = SwapChain::querySupport(device, engine->getSurface());
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

void DeviceManager::createLogicalDevice()
{
  Engine *engine = Engine::GetInstance();
  QueueFamilyIndices indices = VulkanHelpers::findQueueFamilies(engine->getPhysicalDevice(), engine->getSurface());

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};

    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = static_cast<uint32_t>(VulkanHelpers::deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = VulkanHelpers::deviceExtensions.data();

  if (Validation::enableValidationLayers)
  {
    createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanHelpers::validationLayers.size());
    createInfo.ppEnabledLayerNames = VulkanHelpers::validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(engine->getPhysicalDevice(), &createInfo, nullptr, &engine->getDevice()) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create logical device!");
  }

  vkGetDeviceQueue(engine->getDevice(), indices.graphicsFamily.value(), 0, engine->getGraphicsQueue());
  vkGetDeviceQueue(engine->getDevice(), indices.presentFamily.value(), 0, engine->getPresentQueue());
}