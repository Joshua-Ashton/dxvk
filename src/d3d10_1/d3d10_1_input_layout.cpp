#include "d3d10_1_device.h"
#include "d3d10_1_input_layout.h"

namespace dxvk {
  
  D3D10InputLayout::D3D10InputLayout(
          D3D10Device*                pDevice,
          uint32_t                    numAttributes,
    const DxvkVertexAttribute*        pAttributes,
          uint32_t                    numBindings,
    const DxvkVertexBinding*          pBindings)
  : m_device(pDevice) {
    m_attributes.resize(numAttributes);
    m_bindings.resize(numBindings);
    
    for (uint32_t i = 0; i < numAttributes; i++)
      m_attributes.at(i) = pAttributes[i];
    
    for (uint32_t i = 0; i < numBindings; i++)
      m_bindings.at(i) = pBindings[i];
  }
  
  
  D3D10InputLayout::~D3D10InputLayout() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10InputLayout::QueryInterface(REFIID riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(ID3D10DeviceChild)
     || riid == __uuidof(ID3D10InputLayout)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    Logger::warn("D3D10InputLayout::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  
  
  void STDMETHODCALLTYPE D3D10InputLayout::GetDevice(ID3D10Device** ppDevice) {
    *ppDevice = m_device.ref();
  }
  
  
  void D3D10InputLayout::BindToContext(const Rc<DxvkContext>& ctx) {
    ctx->setInputLayout(
      m_attributes.size(),
      m_attributes.data(),
      m_bindings.size(),
      m_bindings.data());
  }
  
}
