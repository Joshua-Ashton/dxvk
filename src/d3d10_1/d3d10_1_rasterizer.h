#pragma once

#include <dxvk_device.h>

#include "d3d10_1_device_child.h"

namespace dxvk {
  
  class D3D10Device;
  
  class D3D10RasterizerState : public D3D10DeviceChild<ID3D10RasterizerState> {
    
  public:
    
    using DescType = D3D10_RASTERIZER_DESC;
    
    D3D10RasterizerState(
            D3D10Device*                    device,
      const D3D10_RASTERIZER_DESC&         desc);
    ~D3D10RasterizerState();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID  riid,
            void**  ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(
            ID3D10Device **ppDevice) final;
    
    void STDMETHODCALLTYPE GetDesc(
            D3D10_RASTERIZER_DESC* pDesc) final;
    
    void BindToContext(
      const Rc<DxvkContext>&  ctx);
    
    static D3D10_RASTERIZER_DESC DefaultDesc();
    
    static HRESULT NormalizeDesc(
            D3D10_RASTERIZER_DESC* pDesc);
    
  private:
    
    D3D10Device* const     m_device;
    D3D10_RASTERIZER_DESC m_desc;
    DxvkRasterizerState    m_state;
    
  };
  
}
