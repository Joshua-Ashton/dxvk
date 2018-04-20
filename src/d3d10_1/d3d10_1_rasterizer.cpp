#include "d3d10_1_device.h"
#include "d3d10_1_rasterizer.h"

namespace dxvk {
  
  D3D10RasterizerState::D3D10RasterizerState(
          D3D10Device*                    device,
    const D3D10_RASTERIZER_DESC&         desc)
  : m_device(device), m_desc(desc) {
    
    // State that is not supported in D3D10
    m_state.enableDiscard     = VK_FALSE;
    
    // Polygon mode. Determines whether the rasterizer fills
    // a polygon or renders lines connecting the vertices.
    m_state.polygonMode = VK_POLYGON_MODE_FILL;
    
    switch (desc.FillMode) {
      case D3D10_FILL_WIREFRAME: m_state.polygonMode = VK_POLYGON_MODE_LINE; break;
      case D3D10_FILL_SOLID:     m_state.polygonMode = VK_POLYGON_MODE_FILL; break;
      
      default:
        Logger::err(str::format(
          "D3D10RasterizerState: Unsupported fill mode: ",
          desc.FillMode));
    }
    
    // Face culling properties. The rasterizer may discard
    // polygons that are facing towards or away from the
    // viewer, depending on the options below.
    m_state.cullMode = VK_CULL_MODE_NONE;
    
    switch (desc.CullMode) {
      case D3D10_CULL_NONE:  m_state.cullMode = VK_CULL_MODE_NONE;      break;
      case D3D10_CULL_FRONT: m_state.cullMode = VK_CULL_MODE_FRONT_BIT; break;
      case D3D10_CULL_BACK:  m_state.cullMode = VK_CULL_MODE_BACK_BIT;  break;
      
      default:
        Logger::err(str::format(
          "D3D10RasterizerState: Unsupported cull mode: ",
          desc.CullMode));
    }
    
    m_state.frontFace = desc.FrontCounterClockwise
      ? VK_FRONT_FACE_COUNTER_CLOCKWISE
      : VK_FRONT_FACE_CLOCKWISE;
    
    // Let's treat the depth bias as enabled by default
    m_state.depthBiasEnable   = VK_TRUE;
    m_state.depthBiasConstant = static_cast<float>(desc.DepthBias);
    m_state.depthBiasClamp    = desc.DepthBiasClamp;
    m_state.depthBiasSlope    = desc.SlopeScaledDepthBias;
    m_state.enableDepthClamp  = desc.DepthClipEnable ? VK_FALSE : VK_TRUE;
    
    if (!desc.DepthClipEnable)
      Logger::warn("D3D10RasterizerState: Depth clamp not properly supported");
    
    if (desc.AntialiasedLineEnable)
      Logger::err("D3D10RasterizerState: Antialiased lines not supported");
  }
  
  
  D3D10RasterizerState::~D3D10RasterizerState() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10RasterizerState::QueryInterface(REFIID riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(ID3D10DeviceChild)
     || riid == __uuidof(ID3D10RasterizerState)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    Logger::warn("D3D10RasterizerState::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  
  
  void STDMETHODCALLTYPE D3D10RasterizerState::GetDevice(ID3D10Device** ppDevice) {
    *ppDevice = ref(m_device);
  }
  
  void STDMETHODCALLTYPE D3D10RasterizerState::GetDesc(D3D10_RASTERIZER_DESC* pDesc) {
    *pDesc = m_desc;
  }
  
  void D3D10RasterizerState::BindToContext(const Rc<DxvkContext>& ctx) {
    ctx->setRasterizerState(m_state);
  }
  
  
  D3D10_RASTERIZER_DESC D3D10RasterizerState::DefaultDesc() {
    D3D10_RASTERIZER_DESC dstDesc;
    dstDesc.FillMode              = D3D10_FILL_SOLID;
    dstDesc.CullMode              = D3D10_CULL_BACK;
    dstDesc.FrontCounterClockwise = FALSE;
    dstDesc.DepthBias             = 0;
    dstDesc.SlopeScaledDepthBias  = 0.0f;
    dstDesc.DepthBiasClamp        = 0.0f;
    dstDesc.DepthClipEnable       = TRUE;
    dstDesc.ScissorEnable         = FALSE;
    dstDesc.MultisampleEnable     = FALSE;
    dstDesc.AntialiasedLineEnable = FALSE;
    return dstDesc;
  }
  
  
  HRESULT D3D10RasterizerState::NormalizeDesc(
          D3D10_RASTERIZER_DESC* pDesc) {
    // TODO validate
    return S_OK;
  }
  
}