#include "MapRenderer.h"

#include <stdexcept>
#include <chrono>
#include "file.h"
#include "resource-descriptor.h"
#include "vertex.h"
#include "engine.h"

void MapRenderer::init()
{
  Engine::GetInstance()->createRenderPass();
  createDescriptorSetLayout();
}

void MapRenderer::drawFrame()
{
  Engine *engine = Engine::GetInstance();
  VkDevice device = engine->getDevice();
  SwapChain &swapChain = engine->getSwapChain();

  std::vector<VkFence> inFlightFences = engine->getInFlightFences();
  std::vector<VkFence> imagesInFlight = engine->getImagesInFlight();

  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(device, engine->getSwapChain().get(), UINT64_MAX, engine->getImageAvailableSemaphore(currentFrame), VK_NULL_HANDLE, &imageIndex);

  if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
  {
    vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
  }

  // Mark the image as now being in use by this frame
  imagesInFlight[imageIndex] = inFlightFences[currentFrame];

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    swapChain.recreate();
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("Failed to acquire swap chain image!");
  }

  updateUniformBuffer(imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {engine->getImageAvailableSemaphore(currentFrame)};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &engine->getCommandBuffer(imageIndex);

  VkSemaphore signalSemaphores[] = {engine->getRenderFinishedSemaphore(currentFrame)};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(device, 1, &inFlightFences[currentFrame]);
  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain.get()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || engine->hasFrameBufferResized())
  {
    engine->setFrameBufferResized(false);
    swapChain.recreate();
  }
  else if (result != VK_SUCCESS)
  {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void MapRenderer::updateUniformBuffer(uint32_t currentImage)
{
  Engine *engine = Engine::GetInstance();

  // static auto startTime = std::chrono::high_resolution_clock::now();
  //
  // auto currentTime = std::chrono::high_resolution_clock::now();
  // float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  //ubo.proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
  //ubo.view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  //ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
  //ubo.proj[1][1] *= -1;

  for (auto sprite : engine->getSprites())
  {
    VkDeviceMemory uniformBufferMemory = sprite.getUniformBufferMemory(currentImage);

    UniformBufferObject ubo{};

    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);
    ubo.proj = glm::mat4(0.5f);

    void *data;
    vkMapMemory(engine->getDevice(), uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(engine->getDevice(), uniformBufferMemory);
  }
}

VkRenderPass MapRenderer::createRenderPass()
{

  Engine *engine = Engine::GetInstance();
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = engine->getSwapChain().getImageFormat();
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkRenderPass renderPass;
  if (vkCreateRenderPass(engine->getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create render pass!");
  }

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  return renderPass;
}

void MapRenderer::createDescriptorSetLayout()
{
  Engine *engine = Engine::GetInstance();
  engine->setDescriptorSetLayout(ResourceDescriptor::createLayout(engine->getDevice()));
}