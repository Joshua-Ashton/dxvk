#pragma once

#include "d3d10_1_device_child.h"

namespace dxvk {
  
  class D3D10Device;
  
  class D3D10InputLayout : public D3D10DeviceChild<ID3D10InputLayout> {
    
  public:
    
    D3D10InputLayout(
            D3D10Device*                pDevice,
            uint32_t                    numAttributes,
      const DxvkVertexAttribute*        pAttributes,
            uint32_t                    numBindings,
      const DxvkVertexBinding*          pBindings);
    
    ~D3D10InputLayout();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID  riid,
            void**  ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(
            ID3D10Device **ppDevice) final;
    
    void BindToContext(
      const Rc<DxvkContext>& ctx);
    
  private:
    
    Com<D3D10Device> m_device;
    
    std::vector<DxvkVertexAttribute> m_attributes;
    std::vector<DxvkVertexBinding>   m_bindings;
    
  };
  
}
