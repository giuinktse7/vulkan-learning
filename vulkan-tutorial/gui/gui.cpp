#include "gui.h"

#include <sstream>

#include "../map_io.h"

#include <imgui.h>
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

// #include <imgui_internal.h>

#include "../graphics/engine.h"
#include "../graphics/vulkan_helpers.h"

#include "../graphics/texture.h"

#include "../item.h"

#include <stack>

// #include <imgui_internal.h>

#include <algorithm>

void GUI::renderItem(ItemType *itemType)
{
  if (itemType == nullptr || !itemType->isValid())
  {
    ImGui::Image(
        (ImTextureID)Texture::getBlackTexture()->getDescriptorSet(),
        {static_cast<float>(32), static_cast<float>(32)});
  }
  else
  {

    auto atlas = itemType->getTextureAtlas();
    auto window = atlas->getTextureWindow(itemType->textureAtlasOffset());

    ImTextureID texture = (ImTextureID)atlas->getDescriptorSet();

    ImVec2 imageSize = {32.0f, 32.0f};

    // Adjust for magnification scaling to scale equally in both X and Y.
    if (atlas->spriteWidth == 64 && atlas->spriteHeight == 32)
    {
      imageSize.y = 16.0f;
    }
    else if (atlas->spriteWidth == 32 && atlas->spriteHeight == 64)
    {
      imageSize.x = 16.0f;
    }

    std::string childId = "" + std::to_string(itemType->id);
    ImU32 black = ImColor(0, 0, 0);
    ImU32 gray = ImColor(0x33, 0x33, 0x33);
    ImGui::PushID(childId.c_str());
    // ImGui::BeginChild(childId.c_str(), {32.0f, 32.0f});
    ImGui::BeginChild(childId.c_str(), {32.0f, 32.0f});

    ImU32 magenta = ImColor(0xFF, 0, 0xFF);
    ImGui::PushStyleColor(ImGuiCol_Button, black);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, gray);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, black);

    ImGui::PushID((int)itemType->id);
    // ImGui::Image(texture, {32.0f, 32.0f}, {window.x0, window.y0}, {window.x1, window.y1});
    if (ImGui::ImageButton(
            texture, imageSize,
            {window.x0, window.y0},
            {window.x1, window.y1},
            0,
            {0, 0, 0, 0},
            {1, 1, 1, 1}))
    {
      this->brushServerId = itemType->id;
    }
    ImGui::PopID();
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::IsItemHovered() ? gray : black);
    ImGui::PopStyleColor();
    ImGui::PopID();
  }
}

void GUI::initialize()
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

  ImGui::StyleColorsLight();
  //ImGui::StyleColorsDark();
  initForVulkan();
}

bool dockOpened = true;

void GUI::initForVulkan()
{

  createDescriptorPool();
  createRenderPass();

  ImGui_ImplGlfw_InitForVulkan(g_engine->getWindow(), true);
  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance = g_engine->getVkInstance();
  initInfo.PhysicalDevice = g_engine->getPhysicalDevice();
  initInfo.Device = g_engine->getDevice();
  initInfo.QueueFamily = 0;
  initInfo.Queue = *g_engine->getGraphicsQueue();
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = descriptorPool;
  initInfo.Allocator = g_engine->getAllocator();
  initInfo.ImageCount = g_engine->getImageCount();
  initInfo.MinImageCount = g_engine->getMinImageCount();
  initInfo.CheckVkResultFn = checkVkResult;
  ImGui_ImplVulkan_Init(&initInfo, renderPass);

  createFontsTexture();

  setupCommandPoolsAndBuffers();
  createFrameBuffers();
}

static void HelpMarker(const char *desc)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

void GUI::createTopMenuBar()
{
  float menuHeight = 90.0f;

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar;

  auto *viewport = ImGui::GetMainViewport();

  ImVec2 size = {viewport->GetWorkSize().x, menuHeight};

  ImGui::SetNextWindowPos({0, 0});
  ImGui::SetNextWindowSize(size);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
  ImGui::SetNextWindowSizeConstraints({-1, menuHeight}, {-1, menuHeight});
  ImGui::Begin("Top menu bar", nullptr, windowFlags);
  ImGui::PopStyleVar(3);

  if (ImGui::BeginMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      // Disabling fullscreen would allow the window to be moved to the front of other windows,
      // which we can't undo at the moment without finer window depth/z control.
      //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);
      ImGui::MenuItem("Ponko", "");
      ImGui::Separator();
      if (ImGui::MenuItem("Save", "Ctrl+S"))
      {
        MapIO::saveMap(*g_engine->getMapRenderer()->map);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit"))
    {
      ImGui::MenuItem("Nothing here yet.");
      ImGui::EndMenu();
    }

    HelpMarker("Map editor. Repository: https://github.com/giuinktse7/vulkan-learning");
    if (ImGui::InputInt("serverIdInput", (int *)&selectedServerId, 1, 20))
    {
      if (selectedServerId < 100)
        selectedServerId = 100;
      // uint16_t currentId = static_cast<uint16_t>(std::max((uint32_t)100, this->selectedServerId));
      // if (!Items::items.getItemType(selectedServerId)->isValid())
      // {
      //   if (this->selectedServerId <= prevId)
      //   {
      //     ItemType *prevType = Items::items.getPreviousValidItemType(currentId);

      //     this->prevId = this->selectedServerId;
      //     this->selectedServerId = prevType == nullptr ? 100 : prevType->id;
      //   }
      //   else
      //   {
      //     this->prevId = this->selectedServerId;
      //     this->selectedServerId = Items::items.getNextValidItemType(currentId)->id;
      //   }
      // }
    }
    else
    {
      // if (selectedServerId >= 100)
      // {
      //   // std::cout << selectedServerId << std::endl;
      //   if (selectedServerId > prevId)
      //   {
      //     this->prevId = this->selectedServerId;
      //     this->selectedServerId = Items::items.getNextValidItemType(selectedServerId)->id;
      //   }

      //   prevId = selectedServerId;
      // }
    }

    ImGui::EndMenuBar();

    renderN(4);
  }
  ImGui::End();
}

void GUI::renderN(uint32_t n)
{
  for (uint32_t i = 0; i < n; ++i)
  {
    renderItem(Items::items.getItemType(this->selectedServerId - n + i));
    ImGui::SameLine();
  }

  for (uint32_t i = 0; i <= n; ++i)
  {
    renderItem(Items::items.getItemType(this->selectedServerId + i));
    if (i < n)
    {
      ImGui::SameLine();
    }
  }

  // std::stack<ItemType *> toBeRendered;
  // ItemType *item = Items::items.getNextValidItemType(this->selectedServerId);
  // for (uint32_t i = 0; i < n; ++i)
  // {
  //   item = item == nullptr ? nullptr : Items::items.getPreviousValidItemType(item->id - 1);
  //   toBeRendered.push(item);
  // }

  // while (!toBeRendered.empty())
  // {
  //   renderItem(toBeRendered.top());
  //   ImGui::SameLine();

  //   toBeRendered.pop();
  // }

  // item = Items::items.getNextValidItemType(this->selectedServerId);
  // renderItem(item);
  // ImGui::SameLine();

  // for (uint32_t i = 0; i < n; ++i)
  // {
  //   item = item == nullptr ? nullptr : Items::items.getNextValidItemType(item->id + 1);
  //   renderItem(item);
  //   if (i < n)
  //   {
  //     ImGui::SameLine();
  //   }
  // }
}

constexpr uint32_t DEFAULT_PADDING = 4.0f;

void GUI::recordFrame(uint32_t currentFrame)
{

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // bool open = true;
  // ImGui::ShowDemoWindow(&open);

  // ImGui::Begin("My First Tool", &testOpen, ImGuiWindowFlags_MenuBar);
  // if (ImGui::BeginMenuBar())
  // {
  //   if (ImGui::BeginMenu("File"))
  //   {
  //     if (ImGui::MenuItem("Open..", "Ctrl+O"))
  //     { /* Do stuff */
  //     }
  //     if (ImGui::MenuItem("Save", "Ctrl+S"))
  //     { /* Do stuff */
  //     }
  //     if (ImGui::MenuItem("Close", "Ctrl+W"))
  //     {
  //       testOpen = false;
  //     }
  //     ImGui::EndMenu();
  //   }
  //   ImGui::EndMenuBar();
  // }

  // // Plot some values
  // const float my_values[] = {0.2f, 0.1f, 1.0f, 0.5f, 0.9f, 2.2f};
  // ImGui::PlotLines("Frame Times", my_values, IM_ARRAYSIZE(my_values));

  // // Display contents in a scrolling region
  // ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
  // ImGui::BeginChild("Scrolling");
  // for (int n = 0; n < 50; n++)
  //   ImGui::Text("%04d: Some text", n);
  // ImGui::EndChild();
  // ImGui::End();

  static bool opt_fullscreen_persistant = true;
  bool opt_fullscreen = opt_fullscreen_persistant;
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

  float padding = DEFAULT_PADDING;

  float bottomBarHeight = 20 + padding * 2;

  // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
  // because it would be confusing to have two docking targets within each others.
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
  if (opt_fullscreen)
  {
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos({0, static_cast<float>(g_engine->getHeight() - bottomBarHeight)});
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  }

  // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
  // and handle the pass-thru hole, so we ask Begin() to not render a background.
  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    window_flags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSizeConstraints({-1, bottomBarHeight}, {-1, bottomBarHeight});
  ImGui::Begin("DockSpace Demo", &dockOpened, window_flags);
  ImGui::PopStyleVar();

  if (opt_fullscreen)
    ImGui::PopStyleVar(2);

  ImGuiViewport *viewport = ImGui::GetMainViewport();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {padding, padding});
  ImGui::PopStyleVar(1);

  ImGui::BeginChild("Bottom1", {viewport->GetWorkSize().x / 4, bottomBarHeight + padding * 2}, ImGuiWindowFlags_NoResize);

  std::string zoom = "Zoom factor: " + std::to_string(g_engine->getMapRenderer()->camera.zoomFactor);
  ImGui::Text(zoom.c_str());

  ImGui::EndChild();

  ImGui::SameLine();

  ImGui::BeginChild("Bottom2", {viewport->GetWorkSize().x / 4, bottomBarHeight + padding * 2}, ImGuiWindowFlags_NoResize);

  auto [x, y, z] = g_engine->screenToGamePos(g_engine->getMousePosition());

  std::ostringstream positionString;
  positionString << "x: " << (x) << " y: " << (y) << " z: " << z;
  ImGui::Text(positionString.str().c_str());

  ImGui::EndChild();

  ImGui::End();

  createTopMenuBar();

  updateCommandPool(currentFrame);

  VkExtent2D extent = g_engine->getSwapChain().getExtent();

  glm::vec4 clearColor = g_engine->clearColor;
  VkClearValue clearValue = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};

  VkRenderPassBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.renderPass = renderPass;
  info.framebuffer = frameBuffers[currentFrame];
  info.renderArea.extent.width = extent.width;
  info.renderArea.extent.height = extent.height;
  info.clearValueCount = 1;
  info.pClearValues = &clearValue;
  vkCmdBeginRenderPass(commandBuffers[currentFrame], &info, VK_SUBPASS_CONTENTS_INLINE);

  ImGui::Render();
  this->captureIO();
}

void GUI::captureIO()
{
  auto io = ImGui::GetIO();
  g_engine->captureMouse = !io.WantCaptureMouse;
  g_engine->captureKeyboard = !io.WantCaptureKeyboard;
}

void GUI::createDescriptorPool()
{

  VkDescriptorPoolSize pool_sizes[] =
      {
          {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
          {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
          {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
          {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
          {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
          {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
          {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
          {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;
  if (vkCreateDescriptorPool(g_engine->getDevice(), &pool_info, g_engine->getAllocator(), &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void GUI::endFrame(uint32_t currentFrame)
{
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);

  vkCmdEndRenderPass(commandBuffers[currentFrame]);

  vkEndCommandBuffer(commandBuffers[currentFrame]);
}

void GUI::createFrameBuffers()
{

  frameBuffers.resize(g_engine->getImageCount());

  VkImageView attachment[1];
  VkFramebufferCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.renderPass = renderPass;
  info.attachmentCount = 1;
  info.pAttachments = attachment;
  info.width = g_engine->getWidth();
  info.height = g_engine->getHeight();
  info.layers = 1;
  for (uint32_t i = 0; i < g_engine->getImageCount(); i++)
  {
    attachment[0] = g_engine->getSwapChain().getImageView(i);
    if (vkCreateFramebuffer(g_engine->getDevice(), &info, g_engine->getAllocator(), &frameBuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void GUI::createRenderPass()
{
  VkAttachmentDescription attachment = {};
  attachment.format = g_engine->getSwapChain().getImageFormat();
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment = {};
  color_attachment.attachment = 0;
  color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0; // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 1;
  info.pAttachments = &attachment;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = 1;
  info.pDependencies = &dependency;
  if (vkCreateRenderPass(g_engine->getDevice(), &info, nullptr, &renderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("Could not create Dear ImGui's render pass");
  }
}

void GUI::createFontsTexture()
{
  VkCommandBuffer commandBuffer = g_engine->beginSingleTimeCommands();
  ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
  g_engine->endSingleTimeCommands(commandBuffer);
}

void GUI::setupCommandPoolsAndBuffers()
{

  size_t size = g_engine->getSwapChain().getImageViewCount();

  commandPools.resize(size);
  commandBuffers.resize(size);
  for (size_t i = 0; i < size; i++)
  {
    VulkanHelpers::createCommandPool(&commandPools[i], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VulkanHelpers::createCommandBuffers(&commandBuffers[i], 1, commandPools[i]);
  }
}

void GUI::checkVkResult(VkResult err)
{
  if (err == 0)
    return;
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0)
    abort();
}

void GUI::updateCommandPool(uint32_t currentFrame)
{

  if (vkResetCommandPool(g_engine->getDevice(), commandPools[currentFrame], 0) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to reset GUI command pool!");
  }

  VkCommandBufferBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(commandBuffers[currentFrame], &info) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin GUI commandBuffer!");
  }
}

void GUI::recreate()
{

  cleanup();

  ImGui_ImplVulkan_SetMinImageCount(g_engine->getMinImageCount());

  createRenderPass();
  setupCommandPoolsAndBuffers();
  createFrameBuffers();
}

void GUI::cleanup()
{

  VkDevice device = g_engine->getDevice();

  for (auto framebuffer : frameBuffers)
  {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  vkDestroyRenderPass(device, renderPass, nullptr);

  for (size_t i = 0; i < g_engine->getMaxFramesInFlight(); ++i)
  {
    auto &commandBuffer = commandBuffers[i];
    if (commandBuffer)
    {
      vkFreeCommandBuffers(
          device,
          commandPools[i],
          1,
          &commandBuffer);

      commandBuffer = nullptr;
    }

    vkDestroyCommandPool(device, commandPools[i], nullptr);
  }
}