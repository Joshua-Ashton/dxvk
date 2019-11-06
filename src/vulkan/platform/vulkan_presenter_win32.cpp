#include "vulkan_presenter.h"

namespace dxvk::vk {

  VkResult Presenter::createPlatformSurface() {
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(
      GetWindowLongPtr(m_window, GWLP_HINSTANCE));

    VkWin32SurfaceCreateInfoKHR info;
    info.sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.pNext      = nullptr;
    info.flags      = 0;
    info.hinstance  = instance;
    info.hwnd       = m_window;
    
    return m_vki->vkCreateWin32SurfaceKHR(
      m_vki->instance(), &info, nullptr, &m_surface);
  }

}