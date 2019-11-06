#include "vulkan_presenter.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

namespace dxvk::vk {

  VkResult Presenter::createPlatformSurface() {
    return SDL_Vulkan_CreateSurface(m_window, m_vki->instance(), &m_surface)
      ? VK_SUCCESS
      : VK_ERROR_OUT_OF_HOST_MEMORY;
  }

}