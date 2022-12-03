#pragma once

#include "IWindow.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

#include <vector>
#include <memory>

class ImGuiVulkanGlfwApplication
{
public: // Interface

  void Run();

  void AddWindow(
      std::shared_ptr<IWindow> && window
    );

  void AddWindow(
      const std::shared_ptr<IWindow> & window
    );

private: // Service

  void Init();
  void MainLoop();
  void Cleanup();

  void SetupGlfwWindow();
  void SetupVulkan();
  void CreateVulkanInstance();
  void SelectGPU();
  void SelectGraphicsQueueFamily();
  void CreateLogicalDevice();
  void CreateDescriptorPool();
  VkSurfaceKHR CreateWindowSurface();
  void CreateFramebuffers(VkSurfaceKHR surface);
  void SetupImGuiContext();
  void SetupImGuiStyle();
  void SetupBackends();
  void UploadFonts();
  bool FrameRender();
  void FramePresent();
  void ShowDockSpace();

private: // Static service

  static void check_vk_result(VkResult err);
  static void glfw_error_callback(
      int          error,
      const char * description
    );
#ifdef IMGUI_VULKAN_DEBUG_REPORT
  static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(
      VkDebugReportFlagsEXT      flags,
      VkDebugReportObjectTypeEXT objectType,
      uint64_t                   object,
      size_t                     location,
      int32_t                    messageCode,
      const char *               pLayerPrefix,
      const char *               pMessage,
      void       *               pUserData
    );
#endif // IMGUI_VULKAN_DEBUG_REPORT

private:

  VkAllocationCallbacks *  m_Allocator         = nullptr;
  VkInstance               m_Instance          = VK_NULL_HANDLE;
  VkPhysicalDevice         m_PhysicalDevice    = VK_NULL_HANDLE;
  VkDevice                 m_Device            = VK_NULL_HANDLE;
  uint32_t                 m_QueueFamily       = (uint32_t)-1;
  VkQueue                  m_Queue             = VK_NULL_HANDLE;
  VkDebugReportCallbackEXT m_DebugReport       = VK_NULL_HANDLE;
  VkPipelineCache          m_PipelineCache     = VK_NULL_HANDLE;
  VkDescriptorPool         m_DescriptorPool    = VK_NULL_HANDLE;
  ImGui_ImplVulkanH_Window m_MainWindowData;
  int                      m_MinImageCount     = 2;
  bool                     m_SwapChainRebuild  = false;
  bool                     m_NeedDefaultLayout = true;

  GLFWwindow * m_Window = nullptr;

  std::vector<std::shared_ptr<IWindow>> m_Windows;
};

