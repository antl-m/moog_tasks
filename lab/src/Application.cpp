#include "Application.h"

#include <imgui_internal.h>

#include <iostream>
#include <cstdlib>

//
// Interface
//

void ImGuiVulkanGlfwApplication::Run()
{
  Init();
  MainLoop();
  Cleanup();
}

void ImGuiVulkanGlfwApplication::AddWindow(
    std::shared_ptr<IWindow> && window
  )
{
  m_Windows.push_back(std::move(window));
}

void ImGuiVulkanGlfwApplication::AddWindow(
    const std::shared_ptr<IWindow> & window
  )
{
  m_Windows.push_back(window);
}

//
// Service
//

void ImGuiVulkanGlfwApplication::Init()
{
  SetupGlfwWindow();
  SetupVulkan();
  auto surface = CreateWindowSurface();
  CreateFramebuffers(surface);

  SetupImGuiContext();
  SetupImGuiStyle();
  SetupBackends();
  UploadFonts();
}

void ImGuiVulkanGlfwApplication::MainLoop()
{
  while (!glfwWindowShouldClose(m_Window))
  {
    glfwPollEvents();

    if (m_SwapChainRebuild)
    {
      int width, height;
      glfwGetFramebufferSize(m_Window, &width, &height);
      if (width > 0 && height > 0)
      {
        ImGui_ImplVulkan_SetMinImageCount(m_MinImageCount);
        ImGui_ImplVulkanH_CreateOrResizeWindow(m_Instance, m_PhysicalDevice, m_Device, &m_MainWindowData, m_QueueFamily, m_Allocator, width, height, m_MinImageCount);
        m_MainWindowData.FrameIndex = 0;
        m_SwapChainRebuild = false;
      }
    }

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ShowDockSpace();

    for (auto & Window : m_Windows)
      Window->Show();

    const auto WasRender = FrameRender();

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }

    // Present Main Platform Window
    if (WasRender)
      FramePresent();
  }
}

void ImGuiVulkanGlfwApplication::Cleanup()
{
  // Cleanup
  const auto err = vkDeviceWaitIdle(m_Device);
  check_vk_result(err);
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  ImGui_ImplVulkanH_DestroyWindow(m_Instance, m_Device, &m_MainWindowData, m_Allocator);

  vkDestroyDescriptorPool(m_Device, m_DescriptorPool, m_Allocator);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
  // Remove the debug report callback
  auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugReportCallbackEXT");
  vkDestroyDebugReportCallbackEXT(m_Instance, m_DebugReport, m_Allocator);
#endif // IMGUI_VULKAN_DEBUG_REPORT

  vkDestroyDevice(m_Device, m_Allocator);
  vkDestroyInstance(m_Instance, m_Allocator);

  glfwDestroyWindow(m_Window);
  glfwTerminate();
}

void ImGuiVulkanGlfwApplication::SetupGlfwWindow()
{
  glfwSetErrorCallback(glfw_error_callback);

  if (!glfwInit())
    throw std::runtime_error("GLFW: Failed to init window\n");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  m_Window = glfwCreateWindow(1920, 1080, "2D Morphing", NULL, NULL);
}

void ImGuiVulkanGlfwApplication::SetupVulkan()
{
  if (!glfwVulkanSupported())
    throw std::runtime_error("GLFW: Vulkan not supported\n");

  CreateVulkanInstance();
  SelectGPU();
  SelectGraphicsQueueFamily();
  CreateLogicalDevice();
  CreateDescriptorPool();
}

void ImGuiVulkanGlfwApplication::CreateVulkanInstance()
{
  uint32_t extensions_count = 0;
  const char ** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);

  VkInstanceCreateInfo create_info = {};

  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.enabledExtensionCount = extensions_count;
  create_info.ppEnabledExtensionNames = extensions;
#ifdef IMGUI_VULKAN_DEBUG_REPORT
  // Enabling validation layers
  const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
  create_info.enabledLayerCount = 1;
  create_info.ppEnabledLayerNames = layers;

  // Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
  const char** extensions_ext = (const char**)malloc(sizeof(const char*) * (extensions_count + 1));
  memcpy(extensions_ext, extensions, extensions_count * sizeof(const char*));
  extensions_ext[extensions_count] = "VK_EXT_debug_report";
  create_info.enabledExtensionCount = extensions_count + 1;
  create_info.ppEnabledExtensionNames = extensions_ext;

  // Create Vulkan Instance
  auto err = vkCreateInstance(&create_info, m_Allocator, &m_Instance);
  check_vk_result(err);
  free(extensions_ext);

  // Get the function pointer (required for any extensions)
  auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugReportCallbackEXT");
  IM_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

  // Setup the debug report callback
  VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
  debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
  debug_report_ci.pfnCallback = debug_report;
  debug_report_ci.pUserData = NULL;
  err = vkCreateDebugReportCallbackEXT(m_Instance, &debug_report_ci, m_Allocator, &m_DebugReport);
  check_vk_result(err);
#else
  // Create Vulkan Instance without any debug feature
  const auto err = vkCreateInstance(&create_info, m_Allocator, &m_Instance);
  check_vk_result(err);
  IM_UNUSED(m_DebugReport);
#endif
}

void ImGuiVulkanGlfwApplication::SelectGPU()
{
  uint32_t gpu_count;
  auto err = vkEnumeratePhysicalDevices(m_Instance, &gpu_count, NULL);
  check_vk_result(err);
  IM_ASSERT(gpu_count > 0);

  VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
  err = vkEnumeratePhysicalDevices(m_Instance, &gpu_count, gpus);
  check_vk_result(err);

  // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
  // most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
  // dedicated GPUs) is out of scope of this sample.
  int use_gpu = 0;
  for (int i = 0; i < (int)gpu_count; i++)
  {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(gpus[i], &properties);
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
      use_gpu = i;
      break;
    }
  }

  m_PhysicalDevice = gpus[use_gpu];
  free(gpus);
}

void ImGuiVulkanGlfwApplication::SelectGraphicsQueueFamily()
{
  uint32_t count;
  vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, NULL);
  VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
  vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, queues);
  for (uint32_t i = 0; i < count; i++)
    if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      m_QueueFamily = i;
      break;
    }
  free(queues);
  IM_ASSERT(m_QueueFamily != (uint32_t)-1);
}

void ImGuiVulkanGlfwApplication::CreateLogicalDevice()
{
  int device_extension_count = 1;
  const char * device_extensions[] = { "VK_KHR_swapchain" };
  const float queue_priority[] = { 1.0f };
  VkDeviceQueueCreateInfo queue_info[1] = {};
  queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info[0].queueFamilyIndex = m_QueueFamily;
  queue_info[0].queueCount = 1;
  queue_info[0].pQueuePriorities = queue_priority;
  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
  create_info.pQueueCreateInfos = queue_info;
  create_info.enabledExtensionCount = device_extension_count;
  create_info.ppEnabledExtensionNames = device_extensions;
  auto err = vkCreateDevice(m_PhysicalDevice, &create_info, m_Allocator, &m_Device);
  check_vk_result(err);
  vkGetDeviceQueue(m_Device, m_QueueFamily, 0, &m_Queue);
}

void ImGuiVulkanGlfwApplication::CreateDescriptorPool()
{
  VkDescriptorPoolSize pool_sizes[] =
  {
      { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };
  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;
  auto err = vkCreateDescriptorPool(m_Device, &pool_info, m_Allocator, &m_DescriptorPool);
  check_vk_result(err);
}

VkSurfaceKHR ImGuiVulkanGlfwApplication::CreateWindowSurface()
{
  VkSurfaceKHR surface;
  VkResult err = glfwCreateWindowSurface(m_Instance, m_Window, m_Allocator, &surface);
  check_vk_result(err);
  return surface;
}

void ImGuiVulkanGlfwApplication::CreateFramebuffers(VkSurfaceKHR surface)
{
  int width, height;
  glfwGetFramebufferSize(m_Window, &width, &height);
  ImGui_ImplVulkanH_Window* wd = &m_MainWindowData;

  wd->Surface = surface;

  // Check for WSI support
  VkBool32 res;
  vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_QueueFamily, wd->Surface, &res);
  if (res != VK_TRUE)
  {
    fprintf(stderr, "Error no WSI support on physical device 0\n");
    exit(-1);
  }

  // Select Surface Format
  const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
  const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
  wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(m_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

  // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
  VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
  VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
  wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(m_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
  //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

  // Create SwapChain, RenderPass, Framebuffer, etc.
  IM_ASSERT(m_MinImageCount >= 2);
  ImGui_ImplVulkanH_CreateOrResizeWindow(m_Instance, m_PhysicalDevice, m_Device, wd, m_QueueFamily, m_Allocator, width, height, m_MinImageCount);
}

void ImGuiVulkanGlfwApplication::SetupImGuiContext()
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
  //io.ConfigViewportsNoAutoMerge = true;
  //io.ConfigViewportsNoTaskBarIcon = true;
  io.ConfigWindowsMoveFromTitleBarOnly = true;
}

void ImGuiVulkanGlfwApplication::SetupImGuiStyle()
{
  ImGui::StyleColorsDark();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
  ImGuiStyle& style = ImGui::GetStyle();
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }
}

void ImGuiVulkanGlfwApplication::SetupBackends()
{
  ImGui_ImplGlfw_InitForVulkan(m_Window, true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = m_Instance;
  init_info.PhysicalDevice = m_PhysicalDevice;
  init_info.Device = m_Device;
  init_info.QueueFamily = m_QueueFamily;
  init_info.Queue = m_Queue;
  init_info.PipelineCache = m_PipelineCache;
  init_info.DescriptorPool = m_DescriptorPool;
  init_info.Subpass = 0;
  init_info.MinImageCount = m_MinImageCount;
  init_info.ImageCount = m_MainWindowData.ImageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = m_Allocator;
  init_info.CheckVkResultFn = check_vk_result;
  ImGui_ImplVulkan_Init(&init_info, m_MainWindowData.RenderPass);
}

void ImGuiVulkanGlfwApplication::UploadFonts()
{
  // Use any command queue
  VkCommandPool command_pool = m_MainWindowData.Frames[m_MainWindowData.FrameIndex].CommandPool;
  VkCommandBuffer command_buffer = m_MainWindowData.Frames[m_MainWindowData.FrameIndex].CommandBuffer;

  auto err = vkResetCommandPool(m_Device, command_pool, 0);
  check_vk_result(err);
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  err = vkBeginCommandBuffer(command_buffer, &begin_info);
  check_vk_result(err);

  ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

  VkSubmitInfo end_info = {};
  end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  end_info.commandBufferCount = 1;
  end_info.pCommandBuffers = &command_buffer;
  err = vkEndCommandBuffer(command_buffer);
  check_vk_result(err);
  err = vkQueueSubmit(m_Queue, 1, &end_info, VK_NULL_HANDLE);
  check_vk_result(err);

  err = vkDeviceWaitIdle(m_Device);
  check_vk_result(err);
  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

bool ImGuiVulkanGlfwApplication::FrameRender()
{
  ImGui::Render();
  ImDrawData* main_draw_data = ImGui::GetDrawData();
  const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);

  if (main_is_minimized)
    return false;

  VkSemaphore image_acquired_semaphore = m_MainWindowData.FrameSemaphores[m_MainWindowData.SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore = m_MainWindowData.FrameSemaphores[m_MainWindowData.SemaphoreIndex].RenderCompleteSemaphore;
  {
    const auto err = vkAcquireNextImageKHR(m_Device, m_MainWindowData.Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &m_MainWindowData.FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
      m_SwapChainRebuild = true;
      return false;
    }
    check_vk_result(err);
  }

  ImGui_ImplVulkanH_Frame* fd = &m_MainWindowData.Frames[m_MainWindowData.FrameIndex];
  {
    auto err = vkWaitForFences(m_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
    check_vk_result(err);

    err = vkResetFences(m_Device, 1, &fd->Fence);
    check_vk_result(err);
  }
  {
    auto err = vkResetCommandPool(m_Device, fd->CommandPool, 0);
    check_vk_result(err);
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
    check_vk_result(err);
  }
  {
    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = m_MainWindowData.RenderPass;
    info.framebuffer = fd->Framebuffer;
    info.renderArea.extent.width = m_MainWindowData.Width;
    info.renderArea.extent.height = m_MainWindowData.Height;
    info.clearValueCount = 1;
    info.pClearValues = &m_MainWindowData.ClearValue;
    vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(main_draw_data, fd->CommandBuffer);

  // Submit command buffer
  vkCmdEndRenderPass(fd->CommandBuffer);
  {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &image_acquired_semaphore;
    info.pWaitDstStageMask = &wait_stage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &fd->CommandBuffer;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &render_complete_semaphore;

    auto err = vkEndCommandBuffer(fd->CommandBuffer);
    check_vk_result(err);
    err = vkQueueSubmit(m_Queue, 1, &info, fd->Fence);
    check_vk_result(err);
  }

  return true;
}

void ImGuiVulkanGlfwApplication::FramePresent()
{
  if (m_SwapChainRebuild)
    return;

  VkSemaphore render_complete_semaphore = m_MainWindowData.FrameSemaphores[m_MainWindowData.SemaphoreIndex].RenderCompleteSemaphore;
  VkPresentInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = &render_complete_semaphore;
  info.swapchainCount = 1;
  info.pSwapchains = &m_MainWindowData.Swapchain;
  info.pImageIndices = &m_MainWindowData.FrameIndex;
  VkResult err = vkQueuePresentKHR(m_Queue, &info);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
  {
    m_SwapChainRebuild = true;
    return;
  }
  check_vk_result(err);
  m_MainWindowData.SemaphoreIndex = (m_MainWindowData.SemaphoreIndex + 1) % m_MainWindowData.ImageCount; // Now we can use the next set of semaphores
}

void ImGuiVulkanGlfwApplication::ShowDockSpace()
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar;
  window_flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::Begin("DockSpaceWindow", nullptr, window_flags);

  ImGui::PopStyleVar(3);

  ImGuiID dockspace_id = ImGui::GetID("DockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

  if (m_NeedDefaultLayout)
  {
    m_NeedDefaultLayout = false;

    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

    ImGuiID DockMainID = dockspace_id;

    std::vector<ImGuiID> ids;

    const float ratio = 1.0f / m_Windows.size();

    for (int i = 0; i < m_Windows.size(); ++i)
    {
      if (i == m_Windows.size() - 1)
        ids.push_back(DockMainID);
      else
      if (i == m_Windows.size() - 2)
        ids.push_back(ImGui::DockBuilderSplitNode(DockMainID, ImGuiDir_Left, 0.5f, nullptr, &DockMainID));
      else
        ids.push_back(ImGui::DockBuilderSplitNode(DockMainID, ImGuiDir_Left, ratio, nullptr, &DockMainID));
    }

    for (int i = 0; i < m_Windows.size(); ++i)
    {
      ImGui::DockBuilderDockWindow(m_Windows[i]->GetWindowNameID().c_str(), ids[i]);
    }

    ImGui::DockBuilderFinish(dockspace_id);
  }

  ImGui::End();
}

//
// Static service
//

void ImGuiVulkanGlfwApplication::check_vk_result(
    VkResult err
  )
{
  if (err == 0)
    return;

  std::cerr << "[vulkan] Error: VkResult = " << err << std::endl;

  if (err < 0)
    std::abort();
}

void ImGuiVulkanGlfwApplication::glfw_error_callback(
    int          error,
    const char * description
  )
{
  std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

#ifdef IMGUI_VULKAN_DEBUG_REPORT
VKAPI_ATTR VkBool32 VKAPI_CALL ImGuiVulkanGlfwApplication::debug_report(
    [[maybe_unused]] VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT             objectType,
    [[maybe_unused]] uint64_t              object,
    [[maybe_unused]] size_t                location,
    [[maybe_unused]] int32_t               messageCode,
    [[maybe_unused]] const char *          pLayerPrefix,
    const char *                           pMessage,
    [[maybe_unused]] void *                pUserData
  )
{
  std::cerr << "[vulkan] Debug report from ObjectType: " << objectType << "\n"
            << "Message: " << pMessage << "\n" << std::endl;

  return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT