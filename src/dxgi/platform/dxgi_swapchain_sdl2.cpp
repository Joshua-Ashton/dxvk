#include "dxgi_factory.h"
#include "dxgi_output.h"
#include "dxgi_swapchain.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "sdl2_helpers.h"

namespace dxvk {


  HRESULT STDMETHODCALLTYPE DxgiSwapChain::GetContainingOutput(IDXGIOutput** ppOutput) {
    InitReturnPtr(ppOutput);

    if (!IsWindow(m_window))
      return DXGI_ERROR_INVALID_CALL;

    if (m_target != nullptr) {
      *ppOutput = m_target.ref();
      return S_OK;
    }

    int32_t displayId = SDL_GetWindowDisplayIndex(m_window);

    if (displayId < 0)
      return DXGI_ERROR_INVALID_CALL;
    
    return GetOutputFromMonitor(monitor_cast(displayId), ppOutput);
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
      SDL_SetWindowSize(m_window, pNewTargetParameters->Width, pNewTargetParameters->Height);
    } else {
      Com<IDXGIOutput> output;
      
      if (FAILED(GetOutputFromMonitor(m_monitor, &output))) {
        Logger::err("DXGI: ResizeTarget: Failed to query containing output");
        return E_FAIL;
      }
      
      // If the swap chain allows it, change the display mode
      if (m_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
        ChangeDisplayMode(output.ptr(), pNewTargetParameters);
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

    // NATIVE TODO: How do we target a specific monitor for this?
    SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);
    
    DXGI_OUTPUT_DESC desc;
    output->GetDesc(&desc);

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
    
    return S_OK;
  }


  HRESULT DxgiSwapChain::RestoreDisplayMode(HMONITOR hMonitor) {
    if (!hMonitor)
      return DXGI_ERROR_INVALID_CALL;

    SDL_SetWindowFullscreen(m_window, 0);

    return S_OK;
  }


}