#include "d3d10_1_device.h"
#include "d3d10_1_sampler.h"
#include "d3d10_1_util.h"

namespace dxvk {
  
  D3D10SamplerState::D3D10SamplerState(
          D3D10Device*        device,
    const D3D10_SAMPLER_DESC& desc)
  : m_device(device), m_desc(desc) {
    DxvkSamplerCreateInfo info;
    
    // While D3D10_FILTER is technically an enum, its value bits
    // can be used to decode the filter properties more efficiently.
    const uint32_t filterBits = static_cast<uint32_t>(desc.Filter);
    
    info.magFilter      = (filterBits & 0x04) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    info.minFilter      = (filterBits & 0x10) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    info.mipmapMode     = (filterBits & 0x01) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.useAnisotropy  = (filterBits & 0x40) ? VK_TRUE : VK_FALSE;
    info.compareToDepth = (filterBits & 0x80) ? VK_TRUE : VK_FALSE;
    
    // Set up the remaining properties, which are
    // stored directly in the sampler description
    info.mipmapLodBias = desc.MipLODBias;
    info.mipmapLodMin  = desc.MinLOD;
    info.mipmapLodMax  = desc.MaxLOD;
    info.maxAnisotropy = static_cast<float>(desc.MaxAnisotropy);
    info.addressModeU  = DecodeAddressMode(desc.AddressU);
    info.addressModeV  = DecodeAddressMode(desc.AddressV);
    info.addressModeW  = DecodeAddressMode(desc.AddressW);
    info.compareOp     = DecodeCompareOp(desc.ComparisonFunc);
    info.borderColor   = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    info.usePixelCoord = VK_FALSE;  // Not supported in D3D10
    
    // Make sure to use a valid anisotropy value
    if (desc.MaxAnisotropy <  1) info.maxAnisotropy =  1.0f;
    if (desc.MaxAnisotropy > 16) info.maxAnisotropy = 16.0f;
    
    // Try to find a matching border color if clamp to border is enabled
    if (info.addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
     || info.addressModeV == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
     || info.addressModeW == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
      info.borderColor = DecodeBorderColor(desc.BorderColor);
    
    m_sampler = device->GetDXVKDevice()->createSampler(info);
  }
  
  
  D3D10SamplerState::~D3D10SamplerState() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10SamplerState::QueryInterface(REFIID riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(ID3D10DeviceChild)
     || riid == __uuidof(ID3D10SamplerState)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    Logger::warn("D3D10SamplerState::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  
  
  void STDMETHODCALLTYPE D3D10SamplerState::GetDevice(ID3D10Device** ppDevice) {
    *ppDevice = ref(m_device);
  }
  
  
  void STDMETHODCALLTYPE D3D10SamplerState::GetDesc(D3D10_SAMPLER_DESC* pDesc) {
    *pDesc = m_desc;
  }
  
  
  HRESULT D3D10SamplerState::NormalizeDesc(D3D10_SAMPLER_DESC* pDesc) {
    const uint32_t filterBits = static_cast<uint32_t>(pDesc->Filter);
    
    if (filterBits & 0xFFFFFF2A) {
      Logger::err(str::format("D3D10SamplerState: Unhandled filter: ", filterBits));
      return E_INVALIDARG;
    }
    
    return S_OK;
  }
  
}
