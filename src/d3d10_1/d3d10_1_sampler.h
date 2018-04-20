#pragma once

#include <dxvk_device.h>

#include "d3d10_1_device_child.h"

namespace dxvk {
  
  class D3D10Device;
  
  class D3D10SamplerState : public D3D10DeviceChild<ID3D10SamplerState> {
    
  public:
    
    using DescType = D3D10_SAMPLER_DESC;
    
    D3D10SamplerState(
            D3D10Device*        device,
      const D3D10_SAMPLER_DESC& desc);
    ~D3D10SamplerState();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID  riid,
            void**  ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(
            ID3D10Device **ppDevice) final;
    
    void STDMETHODCALLTYPE GetDesc(
            D3D10_SAMPLER_DESC* pDesc) final;
    
    Rc<DxvkSampler> GetDXVKSampler() const {
      return m_sampler;
    }
    
    static HRESULT NormalizeDesc(
            D3D10_SAMPLER_DESC* pDesc);
    
  private:
    
    D3D10Device* const m_device;
    D3D10_SAMPLER_DESC m_desc;
    Rc<DxvkSampler>    m_sampler;
    
  };
  
}
