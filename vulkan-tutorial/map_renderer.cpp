#include "map_renderer.h"

#include <stdexcept>

#include "file.h"
#include "graphics/resource-descriptor.h"
#include "graphics/engine.h"
#include "graphics/appearances.h"

#include "ecs/ecs.h"
#include "ecs/item_selection.h"

#include "debug.h"

#include "util.h"

#include "logger.h"

constexpr int GROUND_FLOOR = 7;

constexpr glm::vec4 PreviewCursorColor{0.6f,
                                       0.6f,
                                       0.6f,
                                       0.7f};

constexpr glm::vec4 DefaultColor{1.0f, 1.0f, 1.0f, 1.0f};
constexpr glm::vec4 SelectedColor{0.45f, 0.45f, 0.45f, 1.0f};

void MapRenderer::initialize()
{
  currentFrame = &frames.front();

  createRenderPass();
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
  g_engine->mapMemory(indexStagingBuffer.deviceMemory, 0, indexSize, 0, &data);
  uint16_t *indices = reinterpret_cast<uint16_t *>(data);
  std::array<uint16_t, 6> indexArray{0, 1, 3, 3, 1, 2};

  memcpy(indices, &indexArray, sizeof(indexArray));

  indexBuffer = Buffer::create(
      indexSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkCommandBuffer commandBuffer = g_engine->beginSingleTimeCommands();
  VkBufferCopy copyRegion = {};
  copyRegion.size = indexSize;
  vkCmdCopyBuffer(commandBuffer, indexStagingBuffer.buffer, indexBuffer.buffer, 1, &copyRegion);
  g_engine->endSingleTimeCommands(commandBuffer);

  vkUnmapMemory(g_engine->getDevice(), indexStagingBuffer.deviceMemory);
}

VkCommandBuffer MapRenderer::getCommandBuffer()
{

  return currentFrame->commandBuffer;
}

void MapRenderer::createRenderPass()
{
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = g_engine->getSwapChain().getImageFormat();
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

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

  if (vkCreateRenderPass(g_engine->getDevice(), &renderPassInfo, nullptr, &this->renderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create render pass!");
  }
}

void MapRenderer::createGraphicsPipeline()
{
  std::vector<uint8_t> vertShaderCode = File::read("shaders/vert.spv");
  std::vector<uint8_t> fragShaderCode = File::read("shaders/frag.spv");

  VkShaderModule vertShaderModule = g_engine->createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = g_engine->createShaderModule(fragShaderCode);

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
  viewport.width = (float)g_engine->getWidth();
  viewport.height = (float)g_engine->getHeight();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = g_engine->getSwapChain().getExtent();

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
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  std::array<VkDescriptorSetLayout, 2> layouts = {frameDescriptorSetLayout, textureDescriptorSetLayout};
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.size = sizeof(TextureOffset);
  pushConstantRange.offset = 0;

  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(g_engine->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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

  if (vkCreateGraphicsPipelines(g_engine->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(g_engine->getDevice(), fragShaderModule, nullptr);
  vkDestroyShaderModule(g_engine->getDevice(), vertShaderModule, nullptr);
}

void MapRenderer::drawBatches()
{

  VkDeviceSize offsets[] = {0};
  VkBuffer buffers[] = {nullptr};

  std::array<VkDescriptorSet, 2> descriptorSets = {
      currentFrame->descriptorSet,
      nullptr};

  vkCmdBindIndexBuffer(currentFrame->commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

  for (auto &batch : currentFrame->batchDraw.getBatches())
  {
    if (!batch.isValid())
      break;

    buffers[0] = batch.buffer.buffer;
    vkCmdBindVertexBuffers(currentFrame->commandBuffer, 0, 1, buffers, offsets);

    uint32_t offset = 0;
    for (const auto &descriptorInfo : batch.descriptorIndices)
    {
      descriptorSets[1] = descriptorInfo.descriptor;

      vkCmdBindDescriptorSets(currentFrame->commandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout,
                              0,
                              static_cast<uint32_t>(descriptorSets.size()),
                              descriptorSets.data(),
                              0,
                              nullptr);

      // 4 is vertices for one sprite.
      uint32_t sprites = (descriptorInfo.end - offset + 1) / 4;
      for (uint32_t spriteIndex = 0; spriteIndex < sprites; ++spriteIndex)
      {
        vkCmdDrawIndexed(currentFrame->commandBuffer, 6, 1, 0, offset + spriteIndex * 4, 0);
      }

      offset = descriptorInfo.end + 1;
    }

    batch.invalidate();
  }
}

void MapRenderer::beginRenderPass()
{
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = currentFrame->frameBuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = g_engine->getSwapChain().getExtent();
  renderPassInfo.clearValueCount = 1;
  VkClearValue clearValue = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
  renderPassInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(currentFrame->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void MapRenderer::startCommandBuffer()
{
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer &commandBuffer = currentFrame->commandBuffer;

  if (vkAllocateCommandBuffers(g_engine->getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffer");
  }
}

void MapRenderer::recordFrame(uint32_t frameIndex, MapView &mapView)
{

  this->currentFrame = &frames[frameIndex];
  if (!currentFrame->commandBuffer)
  {
    startCommandBuffer();
  }

  vkResetCommandBuffer(currentFrame->commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(currentFrame->commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer");
  }

  currentFrame->batchDraw.commandBuffer = currentFrame->commandBuffer;

  vkCmdBindPipeline(currentFrame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  updateUniformBuffer(mapView);

  drawMap(mapView);

  currentFrame->batchDraw.prepareDraw();

  beginRenderPass();
  drawBatches();
  vkCmdEndRenderPass(currentFrame->commandBuffer);

  if (vkEndCommandBuffer(currentFrame->commandBuffer) != VK_SUCCESS)
  {
    ABORT_PROGRAM("failed to record command buffer");
  }
}

void MapRenderer::drawMap(const MapView &mapView)
{
  // std::cout << std::endl << "drawMap()" << std::endl;
  const auto mapRect = mapView.getGameBoundingRect();
  int floor = mapView.getZ();

  bool aboveGround = floor <= 7;

  int startZ = aboveGround ? GROUND_FLOOR : MAP_LAYERS - 1;
  int endZ = aboveGround ? floor : GROUND_FLOOR + 1;

  for (int mapZ = startZ; mapZ >= endZ; --mapZ)
  {
    int x1 = mapRect.x1 & ~3;
    int x2 = (mapRect.x2 & ~3) + 4;

    int y1 = mapRect.y1 & ~3;
    int y2 = (mapRect.y2 & ~3) + 4;

    for (int mapX = x1; mapX <= x2; mapX += 4)
    {
      for (int mapY = y1; mapY <= y2; mapY += 4)
      {
        quadtree::Node *node = mapView.getMap()->getLeafUnsafe(mapX, mapY);
        if (!node)
          continue;

        for (int x = 0; x < 4; ++x)
        {
          for (int y = 0; y < 4; ++y)
          {
            TileLocation *tile = node->getTile(mapX + x, mapY + y, mapZ);
            if (tile && tile->hasTile())
            {
              if (mapView.isSelectionMoved())
              {
                drawTile(*tile, mapView);
              }
            }
          }
        }
      }
    }
  }

  drawPreviewCursor(mapView);
  drawMovedSelection(mapView);
}

void MapRenderer::drawMovedSelection(const MapView &mapView)
{
  // g_ecs.getSystem<TileSelectionSystem>().
}

void MapRenderer::drawPreviewCursor(const MapView &mapView)
{
  if (!g_engine->getSelectedServerId().has_value())
    return;

  ItemType &selectedItemType = *Items::items.getItemType(g_engine->getSelectedServerId().value());

  Position pos = g_engine->getCursorPos().worldPos(mapView).mapPos().floor(mapView.getFloor());

  Tile *tile = mapView.getMap()->getTile(pos);

  int elevation = tile ? tile->getTopElevation() : 0;

  ObjectDrawInfo drawInfo;
  drawInfo.appearance = selectedItemType.appearance;
  drawInfo.color = PreviewCursorColor;
  drawInfo.drawOffset = {-elevation, -elevation};
  drawInfo.position = pos;
  drawInfo.textureInfo = selectedItemType.getTextureInfo(pos);

  drawItem(drawInfo);
}

void MapRenderer::drawTile(const TileLocation &tileLocation, const MapView &mapView, uint32_t drawFlags)
{
  auto position = tileLocation.getPosition();
  auto tile = tileLocation.getTile();

  TileSelectionComponent *selection = nullptr;
  if (tile->entity.has_value())
  {
    selection = g_ecs.getComponent<TileSelectionComponent>(tile->entity.value());
  }

  bool drawSelected = drawFlags & ItemDrawFlags::DrawSelected;

  Position selectionMovePosDelta{};
  if (mapView.moveSelectionOrigin.has_value())
  {
    selectionMovePosDelta = g_engine->getCursorPos().toPos(mapView) - mapView.moveSelectionOrigin.value();
  }

  if (tile->getGround())
  {
    bool groundSelected = selection && selection->isGroundSelected();
    if (drawSelected || !groundSelected)
    {
      Item *ground = tile->getGround();
      ObjectDrawInfo info;
      info.appearance = ground->itemType->appearance;
      info.position = position;
      info.color = groundSelected ? SelectedColor : DefaultColor;
      info.textureInfo = ground->getTextureInfo(position);

      if (groundSelected)
      {
        info.position += selectionMovePosDelta;
      }

      drawItem(info);
    }
  }

  DrawOffset drawOffset{0, 0};
  auto &items = tile->getItems();

  // The index i is necessary to check whether the item is selected (can't use range-based loop because of this)
  for (size_t i = 0; i < items.size(); ++i)
  {
    bool itemSelected = selection && selection->isItemIndexSelected(i);
    if (itemSelected && !drawSelected)
    {
      continue;
    }

    Item *item = items.at(i).get();

    ObjectDrawInfo info;
    info.appearance = item->itemType->appearance;
    info.color = selection && selection->isItemIndexSelected(i) ? SelectedColor : DefaultColor;
    info.drawOffset = drawOffset;
    info.position = position;
    info.textureInfo = item->getTextureInfo(position);

    drawItem(info);

    if (item->itemType->hasElevation())
    {
      uint32_t elevation = item->itemType->getElevation();
      drawOffset.x -= elevation;
      drawOffset.y -= elevation;
    }
  }
}

void MapRenderer::updateUniformBuffer(const MapView &mapView)
{
  const Viewport viewport = mapView.getViewport();
  glm::mat4 projection = glm::translate(
      glm::ortho(0.0f, viewport.width * viewport.zoom, viewport.height * viewport.zoom, 0.0f),
      glm::vec3(-std::floor(viewport.offsetX), -std::floor(viewport.offsetY), 0.0f));
  ItemUniformBufferObject uniformBufferObject{projection};

  void *data;
  g_engine->mapMemory(currentFrame->uniformBuffer.deviceMemory, 0, sizeof(ItemUniformBufferObject), 0, &data);
  memcpy(data, &uniformBufferObject, sizeof(ItemUniformBufferObject));
  vkUnmapMemory(g_engine->getDevice(), currentFrame->uniformBuffer.deviceMemory);
}

void MapRenderer::createFrameBuffers()
{
  uint32_t imageViewCount = g_engine->getImageCount();

  for (uint32_t i = 0; i < imageViewCount; ++i)
  {
    VkImageView attachments[] = {
        g_engine->getSwapChain().getImageView(i)};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = g_engine->getWidth();
    framebufferInfo.height = g_engine->getHeight();
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(g_engine->getDevice(), &framebufferInfo, nullptr, &frames[i].frameBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void MapRenderer::drawItem(ObjectDrawInfo &info)
{
  currentFrame->batchDraw.push(info);
}

void MapRenderer::cleanup()
{
  for (auto &frame : frames)
  {
    vkDestroyFramebuffer(g_engine->getDevice(), frame.frameBuffer, nullptr);
    frame.frameBuffer = VK_NULL_HANDLE;
  }

  for (auto &frame : frames)
  {
    vkFreeCommandBuffers(
        g_engine->getDevice(),
        commandPool,
        1,
        &frame.commandBuffer);
    frame.commandBuffer = VK_NULL_HANDLE;
  }

  vkDestroyPipeline(g_engine->getDevice(), graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(g_engine->getDevice(), pipelineLayout, nullptr);
  vkDestroyRenderPass(g_engine->getDevice(), renderPass, nullptr);
}

void MapRenderer::recreate()
{
  cleanup();

  createRenderPass();
  createGraphicsPipeline();
  createFrameBuffers();
}

void MapRenderer::createDescriptorSetLayouts()
{
  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;

  VkDescriptorSetLayoutBinding layoutBinding = {};
  layoutBinding.binding = 0;
  layoutBinding.descriptorCount = 1;
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  layoutInfo.pBindings = &layoutBinding;

  if (vkCreateDescriptorSetLayout(g_engine->getDevice(), &layoutInfo, nullptr, &frameDescriptorSetLayout) != VK_SUCCESS)
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

  if (vkCreateDescriptorSetLayout(g_engine->getDevice(), &layoutInfo, nullptr, &textureDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout");
  }
}

void MapRenderer::createUniformBuffers()
{
  VkDeviceSize bufferSize = sizeof(ItemUniformBufferObject);

  for (size_t i = 0; i < g_engine->getMaxFramesInFlight(); i++)
  {
    frames[i].uniformBuffer = Buffer::create(bufferSize,
                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }
}

void MapRenderer::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  uint32_t descriptorCount = g_engine->getMaxFramesInFlight() * 2;

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = descriptorCount;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MAX_NUM_TEXTURES;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = descriptorCount + MAX_NUM_TEXTURES;

  if (vkCreateDescriptorPool(g_engine->getDevice(), &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void MapRenderer::createDescriptorSets()
{
  uint32_t maxFrames = g_engine->getMaxFramesInFlight();
  std::vector<VkDescriptorSetLayout> layouts(maxFrames, frameDescriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = maxFrames;
  allocInfo.pSetLayouts = layouts.data();

  std::array<VkDescriptorSet, 3> descriptorSets;

  if (vkAllocateDescriptorSets(g_engine->getDevice(), &allocInfo, &descriptorSets[0]) != VK_SUCCESS)
  {
    ABORT_PROGRAM("failed to allocate descriptor sets");
  }

  for (uint32_t i = 0; i < descriptorSets.size(); ++i)
  {
    frames[i].descriptorSet = descriptorSets[i];
  }

  for (size_t i = 0; i < g_engine->getMaxFramesInFlight(); ++i)
  {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = frames[i].uniformBuffer.buffer;
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

    vkUpdateDescriptorSets(g_engine->getDevice(), 1, &descriptorWrites, 0, nullptr);
  }
}

void MapRenderer::createCommandPool()
{
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = g_engine->getQueueFamilyIndices().graphicsFamily.value();
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(g_engine->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics command pool!");
  }
}
