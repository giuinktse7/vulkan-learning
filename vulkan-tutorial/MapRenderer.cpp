#include "MapRenderer.h"

#include <stdexcept>
#include <chrono>
#include "file.h"
#include "resource-descriptor.h"
#include "vertex.h"
#include "engine.h"

#include "file.h"

#include "Logger.h"

#include "resource-descriptor.h"

void MapRenderer::initialize()
{

  Engine *engine = Engine::getInstance();

  renderPass = createRenderPass();
  createDescriptorSetLayouts();
  createGraphicsPipeline();
  createFrameBuffers();
  createCommandPool();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();

  auto maxFrames = engine->getMaxFramesInFlight();

  renderData.commandBuffers.resize(maxFrames);

  renderData.indexBuffers.resize(maxFrames);
  renderData.indexStagingBuffers.resize(maxFrames);
  renderData.vertexBuffers.resize(maxFrames);
  renderData.vertexStagingBuffers.resize(maxFrames);

  uint8_t whitePixel[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  defaultTexture = std::make_shared<Texture>(1, 1, &whitePixel[0]);
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

  std::vector<char> vertShaderCode = File::read("shaders/vert.spv");
  std::vector<char> fragShaderCode = File::read("shaders/frag.spv");

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

void MapRenderer::beginRenderPass()
{
  Engine *engine = Engine::getInstance();

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = getFrameBuffer();
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = engine->getSwapChain().getExtent();
  renderPassInfo.clearValueCount = 1;
  VkClearValue clearValue = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
  renderPassInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(getCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void MapRenderer::endRenderPass()
{
  vkCmdEndRenderPass(getCommandBuffer());
}

void MapRenderer::startCommandBuffer()
{
  Engine *engine = Engine::getInstance();

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer &commandBuffer = getCommandBuffer();

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

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

void MapRenderer::renderFrame(uint32_t frameIndex)
{
  Engine *engine = Engine::getInstance();

  setCurrentFrame(frameIndex);

  // Render map
  auto &commandBuffer = getCommandBuffer();
  vkFreeCommandBuffers(
      engine->getDevice(),
      commandPool,
      1,
      &commandBuffer);
  commandBuffer = nullptr;

  renderData.info.descriptorSet = defaultTexture->getDescriptorSet();
  renderData.info.textureWindow = defaultTexture->getTextureWindow();
  renderData.info.color = {1.0f, 1.0f, 1.0f, 1.0f};

  currentBufferIndex = 0;

  startCommandBuffer();

  updateUniformBuffer();
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
  renderData.uniformBufferObject.projection = glm::ortho(
                                                  0.0f,
                                                  width * zoom,
                                                  height * zoom,
                                                  0.0f) *
                                              translated;

  void *data;
  vkMapMemory(engine->getDevice(), getUniformBuffer().bufferMemory, 0, sizeof(ItemUniformBufferObject), 0, &data);
  memcpy(data, &renderData.uniformBufferObject, sizeof(ItemUniformBufferObject));
  vkUnmapMemory(engine->getDevice(), getUniformBuffer().bufferMemory);
}

void MapRenderer::createFrameBuffers()
{
  Engine *engine = Engine::getInstance();

  uint32_t imageViewCount = engine->getImageCount();
  renderData.frameBuffers.resize(imageViewCount);

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

    if (vkCreateFramebuffer(engine->getDevice(), &framebufferInfo, nullptr, &renderData.frameBuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void MapRenderer::endFrame()
{
  queueCurrentBatch();
  drawBatches();

  if (vkEndCommandBuffer(getCommandBuffer()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer");
  }
}

void MapRenderer::drawBatches()
{

  beginRenderPass();

  VkCommandBuffer &commandBuffer = getCommandBuffer();

  if (!drawCommands.empty())
  {
    int curBuffer = -1;

    for (auto &command : drawCommands)
    {
      if (command.bufferIndex != curBuffer)
      {
        curBuffer = command.bufferIndex;

        VkBuffer vertexBuffers[] = {renderData.getVertexBuffers()[curBuffer].buffer};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(
            commandBuffer,
            0,
            1,
            vertexBuffers,
            offsets);

        vkCmdBindIndexBuffer(
            commandBuffer,
            renderData.getIndexBuffers()[curBuffer].buffer,
            0,
            VK_INDEX_TYPE_UINT16);
      }

      std::array<VkDescriptorSet, 2> descriptorSets = {
          renderData.getDescriptorSet(),
          command.descriptorSet};

      vkCmdBindDescriptorSets(commandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout,
                              0,
                              descriptorSets.size(),
                              descriptorSets.data(),
                              0, nullptr);

      vkCmdDrawIndexed(commandBuffer, command.numIndices, 1, command.baseIndex, 0, 0);
    }
  }

  vkCmdEndRenderPass(commandBuffer);

  drawCommands.clear();
}

void MapRenderer::queueCurrentBatch()
{
  queueDrawCommand();
  copyStagingBuffersToDevice(getCommandBuffer());
  unmapStagingBuffers();

  currentBufferIndex++;
}

void MapRenderer::queueDrawCommand()
{
  if (!renderData.info.isStaged())
  {
    return;
  }

  drawCommands.emplace_back(renderData.info.descriptorSet,
                            renderData.info.indexOffset,
                            renderData.info.indexCount,
                            currentBufferIndex);

  renderData.info.offset();
}

void MapRenderer::copyStagingBuffersToDevice(VkCommandBuffer commandBuffer)
{
  auto &indexWrite = renderData.info.indexWrite;
  auto &vertexWrite = renderData.info.vertexWrite;

  VkDeviceSize indexSize = (indexWrite.cursor - indexWrite.start) * sizeof(uint16_t);
  if (indexSize > 0)
  {
    auto indexBuffer = getIndexBuffer();
    auto indexStagingBuffer = getIndexStagingBuffer();

    VkBufferCopy copyRegion = {};
    copyRegion.size = indexSize;
    vkCmdCopyBuffer(commandBuffer, indexStagingBuffer.buffer, indexBuffer.buffer, 1, &copyRegion);
  }

  VkDeviceSize vertexSize = (vertexWrite.cursor - vertexWrite.start) * sizeof(Vertex);
  if (vertexSize > 0)
  {
    auto vertexBuffer = getVertexBuffer();
    auto vertexStagingBuffer = getVertexStagingBuffer();

    VkBufferCopy copyRegion = {};
    copyRegion.size = vertexSize;
    vkCmdCopyBuffer(commandBuffer, vertexStagingBuffer.buffer, vertexBuffer.buffer, 1, &copyRegion);
  }
}

BoundBuffer &MapRenderer::getVertexBuffer()
{
  auto &buffers = renderData.getVertexBuffers();
  if (currentBufferIndex == buffers.size())
  {
    Logger::info("Creating vertex buffer");

    auto buffer = Buffer::create(
        getVertexBufferSize(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    buffers.push_back(buffer);
  }

  if (currentBufferIndex > buffers.size())
  {
    throw std::runtime_error("current buffer index value out of range");
  }

  return buffers[currentBufferIndex];
}

BoundBuffer &MapRenderer::getIndexBuffer()
{
  auto &buffers = renderData.getIndexBuffers();
  if (currentBufferIndex == buffers.size())
  {
    Logger::info("Creating index buffer");

    auto buffer = Buffer::create(
        getIndexBufferSize(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    buffers.push_back(buffer);
  }

  if (currentBufferIndex > buffers.size())
  {
    throw std::runtime_error("current buffer index value out of range (getIndexBuffer)");
  }

  return buffers[currentBufferIndex];
}

BoundBuffer &MapRenderer::getVertexStagingBuffer()
{
  auto &buffers = renderData.getVertexStagingBuffers();
  if (currentBufferIndex == buffers.size())
  {
    Logger::info("Creating vertex staging buffer");

    auto buffer = Buffer::create(
        getVertexBufferSize(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    buffers.push_back(buffer);
  }

  if (currentBufferIndex > buffers.size())
  {
    throw std::runtime_error("current buffer index value out of range (getVertexStagingBuffer)");
  }

  auto &vsb = buffers[currentBufferIndex];
  return vsb;
}

BoundBuffer &MapRenderer::getIndexStagingBuffer()
{
  auto &buffers = renderData.getIndexStagingBuffers();
  if (currentBufferIndex == buffers.size())
  {
    Logger::info("Creating index staging buffer");

    auto buffer = Buffer::create(
        getIndexBufferSize(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    buffers.push_back(buffer);
  }

  if (currentBufferIndex > buffers.size())
  {
    throw std::runtime_error("current buffer index value out of range (getIndexStagingBuffer)");
  }

  auto &isb = buffers[currentBufferIndex];
  return isb;
}

void MapRenderer::mapStagingBufferMemory()
{
  Engine *engine = Engine::getInstance();
  auto &indexWrite = renderData.info.indexWrite;
  auto &vertexWrite = renderData.info.vertexWrite;

  void *data;

  auto indexStagingBuffer = getIndexStagingBuffer();
  vkMapMemory(engine->getDevice(), indexStagingBuffer.bufferMemory, 0, getIndexBufferSize(), 0, &data);
  indexWrite.start = reinterpret_cast<uint16_t *>(data);
  indexWrite.cursor = indexWrite.start;
  indexWrite.end = indexWrite.start + getMaxNumIndices();

  auto vertexStagingBuffer = getVertexStagingBuffer();
  vkMapMemory(engine->getDevice(), vertexStagingBuffer.bufferMemory, 0, getVertexBufferSize(), 0, &data);
  vertexWrite.start = reinterpret_cast<Vertex *>(data);
  vertexWrite.cursor = vertexWrite.start;
  vertexWrite.end = vertexWrite.start + getMaxNumVertices();

  renderData.info.resetOffset();
}

void MapRenderer::unmapStagingBuffers()
{
  Engine *engine = Engine::getInstance();

  auto &indexWrite = renderData.info.indexWrite;
  auto &vertexWrite = renderData.info.vertexWrite;

  if (!indexWrite.start && !vertexWrite.start)
  {
    return;
  }

  auto indexStagingBuffer = getIndexStagingBuffer();
  vkUnmapMemory(engine->getDevice(), indexStagingBuffer.bufferMemory);
  indexWrite.start = nullptr;
  indexWrite.cursor = nullptr;
  indexWrite.end = nullptr;

  auto vertexStagingBuffer = getVertexStagingBuffer();
  vkUnmapMemory(engine->getDevice(), vertexStagingBuffer.bufferMemory);
  vertexWrite.start = nullptr;
  vertexWrite.cursor = nullptr;
  vertexWrite.end = nullptr;
}

VkDeviceSize MapRenderer::getVertexBufferSize()
{
  return getMaxNumVertices() * sizeof(Vertex);
}

VkDeviceSize MapRenderer::getIndexBufferSize()
{
  return getMaxNumIndices() * sizeof(uint16_t);
}

VkDeviceSize MapRenderer::getMaxNumIndices()
{
  return MAX_VERTICES;
}

VkDeviceSize MapRenderer::getMaxNumVertices()
{
  return MAX_VERTICES;
}

void MapRenderer::drawTriangles(const uint16_t *indices, size_t indexCount, const Vertex *vertices, size_t vertexCount)
{
  auto &indexWrite = renderData.info.indexWrite;
  auto &vertexWrite = renderData.info.vertexWrite;
  auto x = getCommandBuffer();
  if (!getCommandBuffer())
  {
    throw std::runtime_error("no command buffer");
  }

  if (!indexWrite.cursor || !vertexWrite.cursor)
  {
    mapStagingBufferMemory();
  }

  auto vbFull = vertexWrite.cursor + vertexCount >= vertexWrite.end;
  auto ibFull = indexWrite.cursor + indexCount >= indexWrite.end;
  if (vbFull || ibFull)
  {
    queueCurrentBatch();
    mapStagingBufferMemory();
  }

  if (!indexWrite.cursor || !vertexWrite.cursor)
  {
    throw std::runtime_error("write destination is null");
  }

  auto base = static_cast<uint16_t>(vertexWrite.cursor - vertexWrite.start);

  // Both indices and vertices need adjustment while being copied to
  // the index/vertex buffers. The indices need the base added,
  // and the vertices need to be colored, for example.
  auto vertexRead = vertices;
  for (int i = 0; i < vertexCount; ++i)
  {
    vertexWrite.cursor->pos = vertexRead->pos;
    vertexWrite.cursor->color = vertexRead->color * renderData.info.color;
    vertexWrite.cursor->texCoord = vertexRead->texCoord;
    vertexWrite.cursor->blendMode = renderData.info.blendMode;

    vertexWrite.cursor++;
    vertexRead++;
  }

  auto indexRead = indices;
  for (int i = 0; i < indexCount; ++i)
  {
    *indexWrite.cursor = *indexRead + base;
    indexWrite.cursor++;
    indexRead++;
  }

  renderData.info.indexCount += indexCount;
  renderData.info.vertexCount += vertexCount;
}

void MapRenderer::setTexture(std::shared_ptr<Texture> texture)
{
  if (!texture)
  {
    texture = defaultTexture;
  }

  if (texture->getDescriptorSet() != renderData.info.descriptorSet)
  {
    queueDrawCommand();
    renderData.info.descriptorSet = texture->getDescriptorSet();
  }
  renderData.info.textureWindow = texture->getTextureWindow();
}

void MapRenderer::drawSprite(float x, float y, float width, float height)
{
  float worldX = x * SPRITE_SIZE;
  float worldY = y * SPRITE_SIZE;

  TextureWindow *window = &renderData.info.textureWindow;
  glm::vec4 color = renderData.info.color;

  std::array<uint16_t, 6> indices{0, 1, 3, 3, 1, 2};

  std::array<Vertex, 4> vertices{{
      {{worldX, worldY}, color, {window->x0, window->y0}},
      {{worldX, worldY + height}, color, {window->x0, window->y1}},
      {{worldX + width, worldY + height}, color, {window->x1, window->y1}},
      {{worldX + width, worldY}, color, {window->x1, window->y0}},
  }};

  drawTriangles(indices.data(), indices.size(), vertices.data(), vertices.size());
}

void MapRenderer::cleanup()
{
  Engine *engine = Engine::getInstance();

  for (auto framebuffer : renderData.frameBuffers)
  {
    vkDestroyFramebuffer(Engine::getInstance()->getDevice(), framebuffer, nullptr);
  }

  for (auto commandBuffer : renderData.commandBuffers)
  {
    vkFreeCommandBuffers(
        engine->getDevice(),
        commandPool,
        1,
        &commandBuffer);
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

  renderData.uniformBuffers.resize(engine->getMaxFramesInFlight());

  VkDeviceSize bufferSize = sizeof(ItemUniformBufferObject);

  for (size_t i = 0; i < engine->getMaxFramesInFlight(); i++)
  {
    renderData.uniformBuffers[i] = (Buffer::create(bufferSize,
                                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
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

  if (vkCreateDescriptorPool(engine->getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
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

  renderData.descriptorSets.resize(maxFrames);
  if (vkAllocateDescriptorSets(engine->getDevice(), &allocInfo, &renderData.descriptorSets[0]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets");
  }

  for (size_t i = 0; i < engine->getMaxFramesInFlight(); ++i)
  {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = renderData.uniformBuffers[i].buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(ItemUniformBufferObject);

    VkWriteDescriptorSet descriptorWrites = {};
    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = renderData.descriptorSets[i];
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