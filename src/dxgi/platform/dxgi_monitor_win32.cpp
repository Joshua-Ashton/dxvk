#include "dxgi_monitor.h"

namespace dxvk {

  HRESULT GetMonitorDisplayMode(
          HMONITOR                hMonitor,
          DWORD                   ModeNum,
          DXGI_MODE_DESC*         pMode) {
    ::MONITORINFOEXW monInfo;
    monInfo.cbSize = sizeof(monInfo);

    if (!::GetMonitorInfoW(hMonitor, reinterpret_cast<MONITORINFO*>(&monInfo))) {
      Logger::err("DXGI: Failed to query monitor info");
      return E_FAIL;
    }
    
    DEVMODEW devMode = { };
    devMode.dmSize = sizeof(devMode);
    
    if (!::EnumDisplaySettingsW(monInfo.szDevice, ModeNum, &devMode))
      return DXGI_ERROR_NOT_FOUND;
    
    pMode->Width            = devMode.dmPelsWidth;
    pMode->Height           = devMode.dmPelsHeight;
    pMode->RefreshRate      = { devMode.dmDisplayFrequency, 1 };
    pMode->Format           = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // FIXME
    pMode->ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    pMode->Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;
    return S_OK;
  }
  
    
  HRESULT SetMonitorDisplayMode(
          HWND                    hWnd,
          HMONITOR                hMonitor,
    const DXGI_MODE_DESC*         pMode) {
    ::MONITORINFOEXW monInfo;
    monInfo.cbSize = sizeof(monInfo);

    if (!::GetMonitorInfoW(hMonitor, reinterpret_cast<MONITORINFO*>(&monInfo))) {
      Logger::err("DXGI: Failed to query monitor info");
      return E_FAIL;
    }
    
    DEVMODEW devMode = { };
    devMode.dmSize       = sizeof(devMode);
    devMode.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    devMode.dmPelsWidth  = pMode->Width;
    devMode.dmPelsHeight = pMode->Height;
    devMode.dmBitsPerPel = GetMonitorFormatBpp(pMode->Format);
    
    if (pMode->RefreshRate.Numerator != 0)  {
      devMode.dmFields |= DM_DISPLAYFREQUENCY;
      devMode.dmDisplayFrequency = pMode->RefreshRate.Numerator
                                 / pMode->RefreshRate.Denominator;
    }
    
    Logger::info(str::format("DXGI: Setting display mode: ",
      devMode.dmPelsWidth, "x", devMode.dmPelsHeight, "@",
      devMode.dmDisplayFrequency));
    
    LONG status = ::ChangeDisplaySettingsExW(
      monInfo.szDevice, &devMode, nullptr, CDS_FULLSCREEN, nullptr);
    
    return status == DISP_CHANGE_SUCCESSFUL ? S_OK : DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;;
  }
  
  
  void GetWindowClientSize(
          HWND                    hWnd,
          UINT*                   pWidth,
          UINT*                   pHeight) {
    RECT rect = { };
    ::GetClientRect(hWnd, &rect);
    
    if (pWidth)
      *pWidth = rect.right - rect.left;
    
    if (pHeight)
      *pHeight = rect.bottom - rect.top;
  }


  HMONITOR GetPrimaryMonitor() {
    return ::MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
  }

}