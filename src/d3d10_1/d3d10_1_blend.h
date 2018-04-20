#pragma once

#include <dxvk_device.h>

#include "d3d10_1_device_child.h"
#include "d3d10_1_util.h"

namespace dxvk {
  
  class D3D10Device;
  
  class D3D10BlendState : public D3D10DeviceChild<ID3D10BlendState1> {
    
  public:
    
    using DescType = D3D10_BLEND_DESC1;
    
    D3D10BlendState(
            D3D10Device*       device,
      const D3D10_BLEND_DESC1& desc);
    ~D3D10BlendState();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID  riid,
            void**  ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(
            ID3D10Device **ppDevice) final;
    
    void STDMETHODCALLTYPE GetDesc(
            D3D10_BLEND_DESC* pDesc) final;
    
    void STDMETHODCALLTYPE GetDesc1(
            D3D10_BLEND_DESC1* pDesc) final;
    
    void BindToContext(
      const Rc<DxvkContext>&  ctx,
            UINT              sampleMask) const;
    
    static D3D10_BLEND_DESC1 DefaultDesc();
    
    static D3D10_BLEND_DESC1 PromoteDesc(
      const D3D10_BLEND_DESC*   pSrcDesc);
    
    static HRESULT NormalizeDesc(
            D3D10_BLEND_DESC1*  pDesc);
    
  private:
    
    D3D10Device* const            m_device;
    D3D10_BLEND_DESC1             m_desc;
    
    std::array<DxvkBlendMode, 8>  m_blendModes;
    DxvkMultisampleState          m_msState;
    DxvkLogicOpState              m_loState;
    
    static DxvkBlendMode DecodeBlendMode(
      const D3D10_RENDER_TARGET_BLEND_DESC1& BlendDesc);
    
    static VkBlendFactor DecodeBlendFactor(
            D3D10_BLEND BlendFactor,
            bool        IsAlpha);
    
    static VkBlendOp DecodeBlendOp(
            D3D10_BLEND_OP BlendOp);
    
  };
  
}
