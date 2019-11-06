#include "d3d11_context.h"
#include "d3d11_device.h"
#include "d3d11_gdi.h"

namespace dxvk {
  
  D3D11GDISurface::D3D11GDISurface(
          ID3D11Resource*     pResource,
          UINT                Subresource)
  : m_resource    (pResource),
    m_subresource (Subresource),
    m_readback    (nullptr),
    m_hdc         (nullptr),
    m_hbitmap     (nullptr),
    m_acquired    (false) { }


  D3D11GDISurface::~D3D11GDISurface() {
  }

  
  HRESULT D3D11GDISurface::Acquire(BOOL Discard, HDC* phdc) {
    Logger::err("D3D11: GDI Interop not supported on this platform.");
    return DXGI_ERROR_INVALID_CALL;
  }

  
  HRESULT D3D11GDISurface::Release(const RECT* pDirtyRect) {
    Logger::err("D3D11: GDI Interop not supported on this platform.");

    return DXGI_ERROR_INVALID_CALL;
  }


  HRESULT D3D11GDISurface::CreateReadbackResource() { return S_OK; }

}
