#pragma once

#include <dxvk_device.h>

#include "d3d10_1_device_child.h"
#include "d3d10_1_util.h"

namespace dxvk {
  
  class D3D10Device;
  
  class D3D10DepthStencilState : public D3D10DeviceChild<ID3D10DepthStencilState> {
    
  public:
    
    using DescType = D3D10_DEPTH_STENCIL_DESC;
    
    D3D10DepthStencilState(
            D3D10Device*              device,
      const D3D10_DEPTH_STENCIL_DESC& desc);
    ~D3D10DepthStencilState();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID  riid,
            void**  ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(
            ID3D10Device **ppDevice) final;
    
    void STDMETHODCALLTYPE GetDesc(
            D3D10_DEPTH_STENCIL_DESC* pDesc) final;
    
    void BindToContext(
      const Rc<DxvkContext>&  ctx);
    
    static D3D10_DEPTH_STENCIL_DESC DefaultDesc();
    
    static HRESULT NormalizeDesc(
            D3D10_DEPTH_STENCIL_DESC* pDesc);
    
  private:
    
    D3D10Device* const        m_device;
    D3D10_DEPTH_STENCIL_DESC  m_desc;
    DxvkDepthStencilState     m_state;
    
    VkStencilOpState DecodeStencilOpState(
      const D3D10_DEPTH_STENCILOP_DESC& StencilDesc,
      const D3D10_DEPTH_STENCIL_DESC&   Desc) const;
    
    VkStencilOp DecodeStencilOp(
            D3D10_STENCIL_OP            Op) const;
    
  };
  
}
