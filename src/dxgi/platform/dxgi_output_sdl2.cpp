#include "dxgi_adapter.h"
#include "dxgi_factory.h"
#include "dxgi_output.h"
#include "dxgi_swapchain.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "sdl2_helpers.h"

namespace dxvk {

  HRESULT DxgiOutput::GetMonitorNameAndSize(HMONITOR hMonitor, WCHAR (&output)[32], RECT& DesktopCoordinates) {
    int32_t displayId = monitor_cast(hMonitor);
    std::wstring name = L"\\DISPLAY";
                 name += std::to_wstring(displayId);

    std::copy(name.begin(), name.end(), output);
    output[name.length()] = L'\0';

    SDL_DisplayMode mode;
    if (SDL_GetDesktopDisplayMode(displayId, &mode) != 0)
      return DXGI_ERROR_INVALID_CALL;

    DesktopCoordinates.left   = 0;
    DesktopCoordinates.top    = 0;
    DesktopCoordinates.right  = mode.w;
    DesktopCoordinates.bottom = mode.h;

    return S_OK;
  }


  HRESULT STDMETHODCALLTYPE DxgiOutput::GetDisplayModeList1(
          DXGI_FORMAT           EnumFormat,
          UINT                  Flags,
          UINT*                 pNumModes,
          DXGI_MODE_DESC1*      pDesc) {
    int32_t displayId = monitor_cast(m_monitor);

    if (pNumModes == nullptr)
      return DXGI_ERROR_INVALID_CALL;
    
    // Special case, just return zero modes
    if (EnumFormat == DXGI_FORMAT_UNKNOWN) {
      *pNumModes = 0;
      return S_OK;
    }

    uint32_t srcModeId = 0;
    uint32_t dstModeId = 0;
    
    std::vector<DXGI_MODE_DESC1> modeList;

    SDL_DisplayMode devMode = { };
    
    while (SDL_GetDisplayMode(displayId, srcModeId++, &devMode)) {
      // Skip modes with incompatible formats
      if (SDL_BITSPERPIXEL(devMode.format) != GetMonitorFormatBpp(EnumFormat))
        continue;
      
      if (pDesc != nullptr) {
        DXGI_MODE_DESC1 mode;
        mode.Width            = devMode.w;
        mode.Height           = devMode.h;
        mode.RefreshRate      = DXGI_RATIONAL{ UINT(devMode.refresh_rate) * 1000, 1000 };
        mode.Format           = EnumFormat;
        mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
        mode.Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;
        mode.Stereo           = FALSE;
        modeList.push_back(mode);
      }
      
      dstModeId += 1;
    }
    
    // Sort display modes by width, height and refresh rate,
    // in that order. Some games rely on correct ordering.
    std::sort(modeList.begin(), modeList.end(),
      [] (const DXGI_MODE_DESC1& a, const DXGI_MODE_DESC1& b) {
        if (a.Width < b.Width) return true;
        if (a.Width > b.Width) return false;
        
        if (a.Height < b.Height) return true;
        if (a.Height > b.Height) return false;
        
        return (a.RefreshRate.Numerator / a.RefreshRate.Denominator)
             < (b.RefreshRate.Numerator / b.RefreshRate.Denominator);
      });
    
    // If requested, write out the first set of display
    // modes to the destination array.
    if (pDesc != nullptr) {
      for (uint32_t i = 0; i < *pNumModes && i < dstModeId; i++)
        pDesc[i] = modeList[i];
      
      if (dstModeId > *pNumModes)
        return DXGI_ERROR_MORE_DATA;
    }
    
    *pNumModes = dstModeId;
    return S_OK;
  }

}