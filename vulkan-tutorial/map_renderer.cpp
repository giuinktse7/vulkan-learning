#include "map_renderer.h"

#include <stdexcept>
#include <chrono>

#include "file.h"
#include "graphics/resource-descriptor.h"
#include "graphics/engine.h"
#include "graphics/resource-descriptor.h"
#include "graphics/appearances.h"

#include "debug.h"

#include "util.h"

#include "logger.h"

MapRenderer::MapRenderer(std::unique_ptr<Map> map)
{
  this->map = std::move(map);
}

void MapRenderer::initialize()
{

  Engine *engine = Engine::getInstance();
  frame = &frames.front();

  renderPass = createRenderPass();
  createDescriptorSetLayouts();
  createGraphicsPipeline();
  createFrameBuffers();
  createCommandPool();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();

  uint32_t indexSize = 6 * sizeof(uint16_t);

  BoundBuffer indexStagingBuffer = Buffer::create(
      indexSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void *data;
  vkMapMemory(engine->getDevice(), indexStagingBuffer.deviceMemory, 0, indexSize, 0, &data);
  uint16_t *indices = reinterpret_cast<uint16_t *>(data);
  std::array<uint16_t, 6> indexArray{0, 1, 3, 3, 1, 2};

  memcpy(indices, &indexArray, sizeof(indexArray));

  indexBuffer = Buffer::create(
      indexSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkCommandBuffer commandBuffer = engine->beginSingleTimeCommands();
  VkBufferCopy copyRegion = {};
  copyRegion.size = indexSize;
  vkCmdCopyBuffer(commandBuffer, indexStagingBuffer.buffer, indexBuffer.buffer, 1, &copyRegion);
  engine->endSingleTimeCommands(commandBuffer);

  vkUnmapMemory(engine->getDevice(), indexStagingBuffer.deviceMemory);

  auto maxFrames = engine->getMaxFramesInFlight();

  uint8_t whitePixel[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  defaultTexture = std::make_shared<Texture>(1, 1, &whitePixel[0]);
}

void MapRenderer::addTextureAtlas(std::unique_ptr<TextureAtlas> &atlas)
{
  textureAtlasIds.insert(atlas->lastSpriteId);
  textureAtlases[atlas->id] = std::move(atlas);
}

void MapRenderer::loadTextureAtlases()
{
  auto engine = Engine::getInstance();

  auto start = std::chrono::high_resolution_clock::now();
  for (const auto &pair : Appearances::catalogInfo)
  {
    std::unique_ptr<TextureAtlas> atlas = TextureAtlas::fromCatalogInfo(pair.second);
    this->addTextureAtlas(atlas);
  }
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
  std::cout << "Loaded compressed sprites in " << duration << " milliseconds." << std::endl;
}

VkRenderPass MapRenderer::createRenderPass()
{
  Engine *engine = Engine::getInstance();
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = engine->getSwapChain().getImageFormat();
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  // The map renderer should not present anymore since GUI will be drawn on top
  // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VkRenderPass renderPass;
  if (vkCreateRenderPass(engine->getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create render pass!");
  }

  return renderPass;
}

void MapRenderer::createGraphicsPipeline()
{
  Engine *engine = Engine::getInstance();

  std::vector<uint8_t> vertShaderCode = File::read("shaders/vert.spv");
  std::vector<uint8_t> fragShaderCode = File::read("shaders/frag.spv");

  VkShaderModule vertShaderModule = engine->createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = engine->createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)engine->getWidth();
  viewport.height = (float)engine->getHeight();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = engine->getSwapChain().getExtent();

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 1.0f;
  colorBlending.blendConstants[1] = 1.0f;
  colorBlending.blendConstants[2] = 1.0f;
  colorBlending.blendConstants[3] = 1.0f;

  std::array<VkDescriptorSetLayout, 2> layouts = {frameDescriptorSetLayout, textureDescriptorSetLayout};
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = layouts.size();
  pipelineLayoutInfo.pSetLayouts = layouts.data();

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.size = sizeof(TextureOffset);
  pushConstantRange.offset = 0;

  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(engine->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(engine->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(engine->getDevice(), fragShaderModule, nullptr);
  vkDestroyShaderModule(engine->getDevice(), vertShaderModule, nullptr);
}

void MapRenderer::drawBatches()
{
  VkDeviceSize offsets[] = {0};
  VkBuffer buffers[] = {nullptr};

  std::array<VkDescriptorSet, 2> descriptorSets = {
      frame->descriptorSet,
      nullptr};

  for (auto &batch : frame->batchDraw.batches)
  {
    buffers[0] = batch.buffer.buffer;
    vkCmdBindVertexBuffers(frame->commandBuffer, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(frame->commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

    uint32_t offset = 0;
    for (const auto &descriptorInfo : batch.descriptorIndices)
    {
      descriptorSets[1] = descriptorInfo.descriptor;

      vkCmdBindDescriptorSets(frame->commandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout,
                              0,
                              descriptorSets.size(),
                              descriptorSets.data(),
                              0,
                              nullptr);

      // 4 is vertices for one sprite.
      uint32_t sprites = (descriptorInfo.end - offset + 1) / 4;
      for (uint32_t spriteIndex = 0; spriteIndex < sprites; ++spriteIndex)
      {
        vkCmdDrawIndexed(frame->commandBuffer, 6, 1, 0, offset + spriteIndex * 4, 0);
      }

      offset = descriptorInfo.end + 1;
    }
  }
}

void MapRenderer::beginRenderPass()
{
  Engine *engine = Engine::getInstance();

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = frame->frameBuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = engine->getSwapChain().getExtent();
  renderPassInfo.clearValueCount = 1;
  VkClearValue clearValue = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
  renderPassInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(frame->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void MapRenderer::endRenderPass()
{
  vkCmdEndRenderPass(frame->commandBuffer);
}

void MapRenderer::startCommandBuffer()
{
  Engine *engine = Engine::getInstance();

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer &commandBuffer = frame->commandBuffer;

  if (vkAllocateCommandBuffers(engine->getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffer");
  }

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer");
  }

  frame->batchDraw.commandBuffer = commandBuffer;
}

void MapRenderer::recordFrame(uint32_t currentFrame)
{
  Engine *engine = Engine::getInstance();

  frame = &frames[currentFrame];
  if (frame->commandBuffer)
  {
    vkFreeCommandBuffers(engine->getDevice(), commandPool, 1, &frame->commandBuffer);
    frame->commandBuffer = nullptr;
  }

  startCommandBuffer();
  vkCmdBindPipeline(frame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  updateUniformBuffer();

  frame->batchDraw.reset();

  drawMap();
  frame->batchDraw.prepareDraw();
}

void MapRenderer::drawMap()
{
  auto engine = Engine::getInstance();
  for (const auto &tileLocation : map->begin())
  {
    auto position = tileLocation->getPosition();
    auto tile = tileLocation->getTile();

    if (tile->getGround())
    {
      drawItem(*tile->getGround(), position);
    }
    for (const auto &item : tile->getItems())
      drawItem(*item, position);
  }
}

void MapRenderer::updateUniformBuffer()
{
  Engine *engine = Engine::getInstance();

  int width, height;
  glfwGetFramebufferSize(engine->getWindow(), &width, &height);

  float zoom = 1 / camera.zoomFactor;
  auto translated = glm::translate(
      glm::mat4(1),
      glm::vec3(camera.position.x, camera.position.y, 0.0f));
  uniformBufferObject.projection = glm::ortho(
                                       0.0f,
                                       width * zoom,
                                       height * zoom,
                                       0.0f) *
                                   translated;

  void *data;
  vkMapMemory(engine->getDevice(), frame->uniformBuffer.deviceMemory, 0, sizeof(ItemUniformBufferObject), 0, &data);
  memcpy(data, &uniformBufferObject, sizeof(ItemUniformBufferObject));
  vkUnmapMemory(engine->getDevice(), frame->uniformBuffer.deviceMemory);
}

void MapRenderer::createFrameBuffers()
{
  Engine *engine = Engine::getInstance();

  uint32_t imageViewCount = engine->getImageCount();

  for (size_t i = 0; i < imageViewCount; ++i)
  {
    VkImageView attachments[] = {
        engine->getSwapChain().getImageView(i)};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = engine->getWidth();
    framebufferInfo.height = engine->getHeight();
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(engine->getDevice(), &framebufferInfo, nullptr, &frames[i].frameBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void MapRenderer::endFrame()
{
  beginRenderPass();

  drawBatches();

  vkCmdEndRenderPass(frame->commandBuffer);

  if (vkEndCommandBuffer(frame->commandBuffer) != VK_SUCCESS)
  {
    ABORT_PROGRAM("failed to record command buffer");
  }
}

TextureAtlas &MapRenderer::getTextureAtlas(const uint32_t spriteId)
{
  std::cout << spriteId << std::endl;
  auto found = std::lower_bound(textureAtlasIds.begin(), textureAtlasIds.end(), spriteId);
  if (found == textureAtlasIds.end())
  {
    std::cout << "Could not find a sprite sheet for sprite ID " << spriteId << "." << std::endl;
    exit(1);
  }

  uint32_t lastSpriteId = *found;

  return *textureAtlases.at(lastSpriteId);
}

TextureAtlas &MapRenderer::getTextureAtlas(ItemType &itemType)
{
  if (itemType.textureAtlas == nullptr)
  {
    auto appearance = Appearances::getById(itemType.clientId);
    auto fg = appearance.frame_group().at(0);
    auto firstSpriteId = fg.sprite_info().sprite_id().at(0);
    itemType.textureAtlas = &getTextureAtlas(firstSpriteId);
  }

  return *itemType.textureAtlas;
}

void MapRenderer::drawItem(Item &item, Position position)
{
  auto &atlas = getTextureAtlas(*item.itemType);

  frame->batchDraw.push(item, position);
}

void MapRenderer::cleanup()
{
  Engine *engine = Engine::getInstance();

  for (auto &frame : frames)
  {
    vkDestroyFramebuffer(Engine::getInstance()->getDevice(), frame.frameBuffer, nullptr);
  }

  for (auto &frame : frames)
  {
    vkFreeCommandBuffers(
        engine->getDevice(),
        commandPool,
        1,
        &frame.commandBuffer);
  }

  vkDestroyPipeline(engine->getDevice(), graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(engine->getDevice(), pipelineLayout, nullptr);
  vkDestroyRenderPass(engine->getDevice(), renderPass, nullptr);
}

void MapRenderer::recreate()
{
  createRenderPass();
  createGraphicsPipeline();

  createFrameBuffers();
}

void MapRenderer::createDescriptorSetLayouts()
{
  Engine *engine = Engine::getInstance();

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;

  VkDescriptorSetLayoutBinding layoutBinding = {};
  layoutBinding.binding = 0;
  layoutBinding.descriptorCount = 1;
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBinding.pImmutableSamplers = nullptr;
  layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  layoutInfo.pBindings = &layoutBinding;

  if (vkCreateDescriptorSetLayout(engine->getDevice(), &layoutInfo, nullptr, &frameDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout");
  }

  VkDescriptorSetLayoutBinding po = {};
  po.binding = 0;
  po.descriptorCount = 1;
  po.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  po.pImmutableSamplers = nullptr;
  po.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  layoutInfo.pBindings = &po;

  if (vkCreateDescriptorSetLayout(engine->getDevice(), &layoutInfo, nullptr, &textureDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout");
  }
}

void MapRenderer::createUniformBuffers()
{
  Engine *engine = Engine::getInstance();

  VkDeviceSize bufferSize = sizeof(ItemUniformBufferObject);

  for (size_t i = 0; i < engine->getMaxFramesInFlight(); i++)
  {
    frames[i].uniformBuffer = Buffer::create(bufferSize,
                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }
}

void MapRenderer::createDescriptorPool()
{
  Engine *engine = Engine::getInstance();

  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  uint32_t descriptorCount = engine->getMaxFramesInFlight() * 2;

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = descriptorCount;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MAX_NUM_TEXTURES;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = descriptorCount + MAX_NUM_TEXTURES;

  if (vkCreateDescriptorPool(engine->getDevice(), &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void MapRenderer::createDescriptorSets()
{
  Engine *engine = Engine::getInstance();

  uint32_t maxFrames = engine->getMaxFramesInFlight();
  std::vector<VkDescriptorSetLayout> layouts(maxFrames, frameDescriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = maxFrames;
  allocInfo.pSetLayouts = layouts.data();

  std::array<VkDescriptorSet, 3> descriptorSets;

  if (vkAllocateDescriptorSets(engine->getDevice(), &allocInfo, &descriptorSets[0]) != VK_SUCCESS)
  {
    ABORT_PROGRAM("failed to allocate descriptor sets");
  }

  for (uint32_t i = 0; i < descriptorSets.size(); ++i)
  {
    frames[i].descriptorSet = descriptorSets[i];
  }

  for (size_t i = 0; i < engine->getMaxFramesInFlight(); ++i)
  {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = frame->uniformBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(ItemUniformBufferObject);

    VkWriteDescriptorSet descriptorWrites = {};
    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = frames[i].descriptorSet;
    descriptorWrites.dstBinding = 0;
    descriptorWrites.dstArrayElement = 0;
    descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(engine->getDevice(), 1, &descriptorWrites, 0, nullptr);
  }
}

void MapRenderer::createCommandPool()
{
  Engine *engine = Engine::getInstance();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = engine->getQueueFamilyIndices().graphicsFamily.value();
  poolInfo.flags = 0; // Optional

  if (vkCreateCommandPool(engine->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics command pool!");
  }
}