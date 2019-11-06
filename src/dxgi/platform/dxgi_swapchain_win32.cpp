#include "dxgi_factory.h"
#include "dxgi_output.h"
#include "dxgi_swapchain.h"

namespace dxvk {


  HRESULT STDMETHODCALLTYPE DxgiSwapChain::GetContainingOutput(IDXGIOutput** ppOutput) {
    InitReturnPtr(ppOutput);
    
    if (!IsWindow(m_window))
      return DXGI_ERROR_INVALID_CALL;
    
    if (m_target != nullptr) {
      *ppOutput = m_target.ref();
      return S_OK;
    }

    RECT windowRect = { 0, 0, 0, 0 };
    ::GetWindowRect(m_window, &windowRect);
    
    HMONITOR monitor = ::MonitorFromPoint(
      { (windowRect.left + windowRect.right) / 2,
        (windowRect.top + windowRect.bottom) / 2 },
      MONITOR_DEFAULTTOPRIMARY);
    
    return GetOutputFromMonitor(monitor, ppOutput);
  }


  HRESULT STDMETHODCALLTYPE DxgiSwapChain::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters) {
    std::lock_guard<std::recursive_mutex> lock(m_lockWindow);

    if (pNewTargetParameters == nullptr)
      return DXGI_ERROR_INVALID_CALL;
    
    if (!IsWindow(m_window))
      return DXGI_ERROR_INVALID_CALL;

    // Update the swap chain description
    if (pNewTargetParameters->RefreshRate.Numerator != 0)
      m_descFs.RefreshRate = pNewTargetParameters->RefreshRate;
    
    m_descFs.ScanlineOrdering = pNewTargetParameters->ScanlineOrdering;
    m_descFs.Scaling          = pNewTargetParameters->Scaling;
    
    if (m_descFs.Windowed) {
      // Adjust window position and size
      RECT newRect = { 0, 0, 0, 0 };
      RECT oldRect = { 0, 0, 0, 0 };
      
      ::GetWindowRect(m_window, &oldRect);
      ::SetRect(&newRect, 0, 0, pNewTargetParameters->Width, pNewTargetParameters->Height);
      ::AdjustWindowRectEx(&newRect,
        ::GetWindowLongW(m_window, GWL_STYLE), FALSE,
        ::GetWindowLongW(m_window, GWL_EXSTYLE));
      ::SetRect(&newRect, 0, 0, newRect.right - newRect.left, newRect.bottom - newRect.top);
      ::OffsetRect(&newRect, oldRect.left, oldRect.top);    
      ::MoveWindow(m_window, newRect.left, newRect.top,
          newRect.right - newRect.left, newRect.bottom - newRect.top, TRUE);
    } else {
      Com<IDXGIOutput> output;
      
      if (FAILED(GetOutputFromMonitor(m_monitor, &output))) {
        Logger::err("DXGI: ResizeTarget: Failed to query containing output");
        return E_FAIL;
      }
      
      // If the swap chain allows it, change the display mode
      if (m_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
        ChangeDisplayMode(output.ptr(), pNewTargetParameters);
      
      // Resize and reposition the window to 
      DXGI_OUTPUT_DESC desc;
      output->GetDesc(&desc);
      
      RECT newRect = desc.DesktopCoordinates;
      
      ::MoveWindow(m_window, newRect.left, newRect.top,
          newRect.right - newRect.left, newRect.bottom - newRect.top, TRUE);
    }
    
    return S_OK;
  }


  HRESULT DxgiSwapChain::EnterFullscreenMode(IDXGIOutput* pTarget) {
    Com<IDXGIOutput> output = pTarget;

    if (!IsWindow(m_window))
      return DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;
    
    if (output == nullptr) {
      if (FAILED(GetContainingOutput(&output))) {
        Logger::err("DXGI: EnterFullscreenMode: Cannot query containing output");
        return E_FAIL;
      }
    }
    
    // Find a display mode that matches what we need
    ::GetWindowRect(m_window, &m_windowState.rect);
    
    if (m_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH) {
      DXGI_MODE_DESC displayMode;
      displayMode.Width            = m_desc.Width;
      displayMode.Height           = m_desc.Height;
      displayMode.RefreshRate      = m_descFs.RefreshRate;
      displayMode.Format           = m_desc.Format;
      // Ignore these two, games usually use them wrong and we don't
      // support any scaling modes except UNSPECIFIED anyway.
      displayMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
      displayMode.Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;
      
      if (FAILED(ChangeDisplayMode(output.ptr(), &displayMode))) {
        Logger::err("DXGI: EnterFullscreenMode: Failed to change display mode");
        return DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;
      }
    }
    
    // Update swap chain description
    m_descFs.Windowed = FALSE;
    
    // Change the window flags to remove the decoration etc.
    LONG style   = ::GetWindowLongW(m_window, GWL_STYLE);
    LONG exstyle = ::GetWindowLongW(m_window, GWL_EXSTYLE);
    
    m_windowState.style = style;
    m_windowState.exstyle = exstyle;
    
    style   &= ~WS_OVERLAPPEDWINDOW;
    exstyle &= ~WS_EX_OVERLAPPEDWINDOW;
    
    ::SetWindowLongW(m_window, GWL_STYLE, style);
    ::SetWindowLongW(m_window, GWL_EXSTYLE, exstyle);
    
    // Move the window so that it covers the entire output
    DXGI_OUTPUT_DESC desc;
    output->GetDesc(&desc);
    
    const RECT rect = desc.DesktopCoordinates;
    
    ::SetWindowPos(m_window, HWND_TOPMOST,
      rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
      SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    
    m_monitor = desc.Monitor;
    m_target  = std::move(output);

    // Apply current gamma curve of the output
    DXGI_VK_MONITOR_DATA* monitorInfo = nullptr;

    if (SUCCEEDED(AcquireMonitorData(m_monitor, &monitorInfo))) {
      if (!monitorInfo->pSwapChain)
        monitorInfo->pSwapChain = this;
      
      SetGammaControl(DXGI_VK_GAMMA_CP_COUNT, monitorInfo->GammaCurve.GammaCurve);
      ReleaseMonitorData();
    }

    return S_OK;
  }
  
  
  HRESULT DxgiSwapChain::LeaveFullscreenMode() {
    if (FAILED(RestoreDisplayMode(m_monitor)))
      Logger::warn("DXGI: LeaveFullscreenMode: Failed to restore display mode");
    
    // Reset gamma control and decouple swap chain from monitor
    DXGI_VK_MONITOR_DATA* monitorInfo = nullptr;

    if (SUCCEEDED(AcquireMonitorData(m_monitor, &monitorInfo))) {
      if (monitorInfo->pSwapChain == this)
        monitorInfo->pSwapChain = nullptr;
      
      SetGammaControl(0, nullptr);
      ReleaseMonitorData();
    }
    
    // Restore internal state
    m_descFs.Windowed = TRUE;
    m_monitor = nullptr;
    m_target  = nullptr;
    
    if (!IsWindow(m_window))
      return S_OK;
    
    // Only restore the window style if the application hasn't
    // changed them. This is in line with what native DXGI does.
    LONG curStyle   = ::GetWindowLongW(m_window, GWL_STYLE) & ~WS_VISIBLE;
    LONG curExstyle = ::GetWindowLongW(m_window, GWL_EXSTYLE) & ~WS_EX_TOPMOST;
    
    if (curStyle == (m_windowState.style & ~(WS_VISIBLE | WS_OVERLAPPEDWINDOW))
     && curExstyle == (m_windowState.exstyle & ~(WS_EX_TOPMOST | WS_EX_OVERLAPPEDWINDOW))) {
      ::SetWindowLongW(m_window, GWL_STYLE,   m_windowState.style);
      ::SetWindowLongW(m_window, GWL_EXSTYLE, m_windowState.exstyle);
    }
    
    // Restore window position and apply the style
    const RECT rect = m_windowState.rect;
    
    ::SetWindowPos(m_window, 0,
      rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
      SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
    
    return S_OK;
  }


  HRESULT DxgiSwapChain::RestoreDisplayMode(HMONITOR hMonitor) {
    if (!hMonitor)
      return DXGI_ERROR_INVALID_CALL;
    
    // Restore registry settings
    DXGI_MODE_DESC mode;
    
    HRESULT hr = GetMonitorDisplayMode(
      hMonitor, ENUM_REGISTRY_SETTINGS, &mode);
    
    if (FAILED(hr))
      return hr;
    
    return SetMonitorDisplayMode(m_window, hMonitor, &mode);
  }


}