#include "dxgi_adapter.h"
#include "dxgi_factory.h"
#include "dxgi_output.h"
#include "dxgi_swapchain.h"

namespace dxvk {

  HRESULT DxgiOutput::GetMonitorNameAndSize(HMONITOR hMonitor, WCHAR (&output)[32], RECT& DesktopCoordinates) {
    ::MONITORINFOEXW monInfo;
    monInfo.cbSize = sizeof(monInfo);

    if (!::GetMonitorInfoW(m_monitor, reinterpret_cast<MONITORINFO*>(&monInfo))) {
      Logger::err("DXGI: Failed to query monitor info");
      return E_FAIL;
    }
    
    std::memcpy(output, monInfo.szDevice, std::size(output));
    DesktopCoordinates = monInfo.rcMonitor;

    return D3D_OK;
  }


  HRESULT STDMETHODCALLTYPE DxgiOutput::GetDisplayModeList1(
          DXGI_FORMAT           EnumFormat,
          UINT                  Flags,
          UINT*                 pNumModes,
          DXGI_MODE_DESC1*      pDesc) {
    if (pNumModes == nullptr)
      return DXGI_ERROR_INVALID_CALL;
    
    // Special case, just return zero modes
    if (EnumFormat == DXGI_FORMAT_UNKNOWN) {
      *pNumModes = 0;
      return S_OK;
    }

    // Query monitor info to get the device name
    ::MONITORINFOEXW monInfo;
    monInfo.cbSize = sizeof(monInfo);

    if (!::GetMonitorInfoW(m_monitor, reinterpret_cast<MONITORINFO*>(&monInfo))) {
      Logger::err("DXGI: Failed to query monitor info");
      return E_FAIL;
    }
    
    // Walk over all modes that the display supports and
    // return those that match the requested format etc.
    DEVMODEW devMode = { };
    devMode.dmSize = sizeof(DEVMODEW);
    
    uint32_t srcModeId = 0;
    uint32_t dstModeId = 0;
    
    std::vector<DXGI_MODE_DESC1> modeList;
    
    while (::EnumDisplaySettingsW(monInfo.szDevice, srcModeId++, &devMode)) {
      // Skip interlaced modes altogether
      if (devMode.dmDisplayFlags & DM_INTERLACED)
        continue;
      
      // Skip modes with incompatible formats
      if (devMode.dmBitsPerPel != GetMonitorFormatBpp(EnumFormat))
        continue;
      
      if (pDesc != nullptr) {
        DXGI_MODE_DESC1 mode;
        mode.Width            = devMode.dmPelsWidth;
        mode.Height           = devMode.dmPelsHeight;
        mode.RefreshRate      = { devMode.dmDisplayFrequency * 1000, 1000 };
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