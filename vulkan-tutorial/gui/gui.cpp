#include "gui.h"
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "../graphics/engine.h"
#include "../graphics/vulkan_helpers.h"

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
  Engine *engine = Engine::getInstance();

  createDescriptorPool();
  createRenderPass();

  ImGui_ImplGlfw_InitForVulkan(engine->getWindow(), true);
  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance = engine->getVkInstance();
  initInfo.PhysicalDevice = engine->getPhysicalDevice();
  initInfo.Device = engine->getDevice();
  initInfo.QueueFamily = 0;
  initInfo.Queue = *engine->getGraphicsQueue();
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = descriptorPool;
  initInfo.Allocator = engine->getAllocator();
  initInfo.ImageCount = engine->getImageCount();
  initInfo.MinImageCount = engine->getMinImageCount();
  initInfo.CheckVkResultFn = checkVkResult;
  ImGui_ImplVulkan_Init(&initInfo, renderPass);

  createFontsTexture();

  setupCommandPoolsAndBuffers();
  createFrameBuffers();
}

void GUI::startFrame(uint32_t currentFrame)
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();

  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
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

void GUI::renderFrame(uint32_t currentFrame)
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("My First Tool", &testOpen, ImGuiWindowFlags_MenuBar);
  if (ImGui::BeginMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Open..", "Ctrl+O"))
      { /* Do stuff */
      }
      if (ImGui::MenuItem("Save", "Ctrl+S"))
      { /* Do stuff */
      }
      if (ImGui::MenuItem("Close", "Ctrl+W"))
      {
        testOpen = false;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  // Plot some values
  const float my_values[] = {0.2f, 0.1f, 1.0f, 0.5f, 0.9f, 2.2f};
  ImGui::PlotLines("Frame Times", my_values, IM_ARRAYSIZE(my_values));

  // Display contents in a scrolling region
  ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
  ImGui::BeginChild("Scrolling");
  for (int n = 0; n < 50; n++)
    ImGui::Text("%04d: Some text", n);
  ImGui::EndChild();
  ImGui::End();

  // ImGui::ShowDemoWindow();

  static bool opt_fullscreen_persistant = true;
  bool opt_fullscreen = opt_fullscreen_persistant;
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

  // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
  // because it would be confusing to have two docking targets within each others.
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  if (opt_fullscreen)
  {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
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

  // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
  // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
  // all active windows docked into it will lose their parent and become undocked.
  // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
  // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSizeConstraints({-1, 80}, {-1, 80});
  ImGui::Begin("DockSpace Demo", &dockOpened, window_flags);
  ImGui::PopStyleVar();

  if (opt_fullscreen)
    ImGui::PopStyleVar(2);

  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
  {
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
  }

  if (ImGui::BeginMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      // Disabling fullscreen would allow the window to be moved to the front of other windows,
      // which we can't undo at the moment without finer window depth/z control.
      //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);
      ImGui::MenuItem("Ponko", "");
      ImGui::Separator();
      if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))
        dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
      if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))
        dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
      if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
        dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
      if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
        dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
      if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
        dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
      ImGui::Separator();
      if (ImGui::MenuItem("Close DockSpace", NULL, false, dockOpened != NULL))
        dockOpened = false;
      ImGui::EndMenu();
    }
    HelpMarker(
        "When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!"
        "\n\n"
        " > if io.ConfigDockingWithShift==false (default):"
        "\n"
        "   drag windows from title bar to dock"
        "\n"
        " > if io.ConfigDockingWithShift==true:"
        "\n"
        "   drag windows from anywhere and hold Shift to dock"
        "\n\n"
        "This demo app has nothing to do with it!"
        "\n\n"
        "This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)."
        "\n\n"
        "ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame."
        "\n\n"
        "(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)");

    ImGui::EndMenuBar();
  }

  ImGui::End();

  updateCommandPool(currentFrame);

  Engine *engine = Engine::getInstance();
  VkExtent2D extent = engine->getSwapChain().getExtent();

  glm::vec4 clearColor = engine->clearColor;
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
}

void GUI::createDescriptorPool()
{
  Engine *engine = Engine::getInstance();

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
  if (vkCreateDescriptorPool(engine->getDevice(), &pool_info, engine->getAllocator(), &descriptorPool) != VK_SUCCESS)
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
  Engine *engine = Engine::getInstance();

  frameBuffers.resize(engine->getImageCount());

  VkImageView attachment[1];
  VkFramebufferCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.renderPass = renderPass;
  info.attachmentCount = 1;
  info.pAttachments = attachment;
  info.width = engine->getWidth();
  info.height = engine->getHeight();
  info.layers = 1;
  for (uint32_t i = 0; i < engine->getImageCount(); i++)
  {
    attachment[0] = engine->getSwapChain().getImageView(i);
    if (vkCreateFramebuffer(engine->getDevice(), &info, engine->getAllocator(), &frameBuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void GUI::createRenderPass()
{
  Engine *engine = Engine::getInstance();
  VkAttachmentDescription attachment = {};
  attachment.format = engine->getSwapChain().getImageFormat();
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
  if (vkCreateRenderPass(engine->getDevice(), &info, nullptr, &renderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("Could not create Dear ImGui's render pass");
  }
}

void GUI::createFontsTexture()
{
  Engine *engine = Engine::getInstance();
  VkCommandBuffer commandBuffer = engine->beginSingleTimeCommands();
  ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
  engine->endSingleTimeCommands(commandBuffer);
}

void GUI::setupCommandPoolsAndBuffers()
{
  Engine *engine = Engine::getInstance();

  uint32_t size = engine->getSwapChain().getImageViewCount();

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

  if (vkResetCommandPool(Engine::getInstance()->getDevice(), commandPools[currentFrame], 0) != VK_SUCCESS)
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
  Engine *engine = Engine::getInstance();

  cleanup();

  ImGui_ImplVulkan_SetMinImageCount(engine->getMinImageCount());

  createRenderPass();
  setupCommandPoolsAndBuffers();
  createFrameBuffers();
}

void GUI::cleanup()
{
  Engine *engine = Engine::getInstance();

  VkDevice device = engine->getDevice();

  for (auto framebuffer : frameBuffers)
  {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  vkDestroyRenderPass(device, renderPass, nullptr);

  for (size_t i = 0; i < engine->getMaxFramesInFlight(); ++i)
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