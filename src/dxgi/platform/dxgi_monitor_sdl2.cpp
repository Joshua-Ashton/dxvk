#include "dxgi_monitor.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "sdl2_helpers.h"

namespace dxvk {

  HRESULT GetMonitorDisplayMode(
          HMONITOR                hMonitor,
          DWORD                   ModeNum,
          DXGI_MODE_DESC*         pMode) {
    int32_t displayCount = SDL_GetNumVideoDisplays();
    int32_t displayId    = monitor_cast(hMonitor);

    SDL_DisplayMode mode = { };

    if (displayId >= displayCount)
      return DXGI_ERROR_NOT_FOUND;

    if (ModeNum == ENUM_CURRENT_SETTINGS && SDL_GetCurrentDisplayMode(displayId, &mode) != 0)
        return DXGI_ERROR_NOT_FOUND;
    else if (ModeNum == ENUM_REGISTRY_SETTINGS && SDL_GetDesktopDisplayMode(displayId, &mode) != 0)
        return DXGI_ERROR_NOT_FOUND;
    else if (SDL_GetDisplayMode(displayId, ModeNum, &mode) != 0)
        return DXGI_ERROR_NOT_FOUND;

    if (SDL_GetDisplayMode(displayId, ModeNum, &mode) != 0)
      return DXGI_ERROR_NOT_FOUND;
    
    pMode->Width            = mode.w;
    pMode->Height           = mode.h;
    pMode->RefreshRate      = { UINT(mode.refresh_rate), 1 };
    pMode->Format           = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // FIXME
    pMode->ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    pMode->Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;
    return S_OK;
  }
  
    
  HRESULT SetMonitorDisplayMode(
          HWND                    hWnd,
          HMONITOR                hMonitor,
    const DXGI_MODE_DESC*         pMode) {
    int32_t displayCount = SDL_GetNumVideoDisplays();
    int32_t displayId    = monitor_cast(hMonitor);

    if (displayId >= displayCount)
      return DXGI_ERROR_NOT_FOUND;

    SDL_DisplayMode wantedMode = { };
    wantedMode.w = pMode->Width;
    wantedMode.h = pMode->Height;

    if (pMode->RefreshRate.Numerator != 0)
      wantedMode.refresh_rate = pMode->RefreshRate.Numerator / pMode->RefreshRate.Denominator;

    SDL_DisplayMode mode = { };
    if (SDL_GetClosestDisplayMode(displayId, &wantedMode, &mode) == nullptr)
      return DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;

    Logger::info(str::format("DXGI: Setting display mode: ",
      mode.w, "x", mode.h, "@",
      mode.refresh_rate));

    if (SDL_SetWindowDisplayMode(hWnd, &mode) != 0)
      return DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;

    return S_OK;
  }
  
  
  void GetWindowClientSize(
          HWND                    hWnd,
          UINT*                   pWidth,
          UINT*                   pHeight) {
    int32_t w, h;
    SDL_GetWindowSize(hWnd, &w, &h);
    
    if (pWidth)
      *pWidth = w;
    
    if (pHeight)
      *pHeight = h;
  }


  HMONITOR GetPrimaryMonitor() {
    return HMONITOR(0);
  }

}