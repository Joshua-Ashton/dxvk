#include "d3d10_1_blend.h"
#include "d3d10_1_device.h"

namespace dxvk {
  
  D3D10BlendState::D3D10BlendState(
          D3D10Device*        device,
    const D3D10_BLEND_DESC1&  desc)
  : m_device(device), m_desc(desc) {
    // If Independent Blend is disabled, we must ignore the
    // blend modes for render target 1 to 7. In Vulkan, all
    // blend modes need to be identical in that case.
    for (uint32_t i = 0; i < m_blendModes.size(); i++) {
      m_blendModes.at(i) = DecodeBlendMode(
        desc.IndependentBlendEnable
          ? desc.RenderTarget[i]
          : desc.RenderTarget[0]);
    }
    
    // Multisample state is part of the blend state in D3D10
    m_msState.sampleMask            = 0; // Set during bind
    m_msState.enableAlphaToCoverage = desc.AlphaToCoverageEnable;
    m_msState.enableAlphaToOne      = VK_FALSE;
    
    m_loState.enableLogicOp         = false;
    m_loState.logicOp               = VK_LOGIC_OP_NO_OP;
  }
  
  
  D3D10BlendState::~D3D10BlendState() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10BlendState::QueryInterface(REFIID riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(ID3D10DeviceChild)
     || riid == __uuidof(ID3D10BlendState)
     || riid == __uuidof(ID3D10BlendState1)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    Logger::warn("D3D10BlendState::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  
  
  void STDMETHODCALLTYPE D3D10BlendState::GetDevice(ID3D10Device** ppDevice) {
    *ppDevice = ref(m_device);
  }
  
  
  void STDMETHODCALLTYPE D3D10BlendState::GetDesc(D3D10_BLEND_DESC* pDesc) {
    pDesc->AlphaToCoverageEnable  = m_desc.AlphaToCoverageEnable;
    
    for (uint32_t i = 0; i < 8; i++) {
      pDesc->BlendEnable[i]           = m_desc.RenderTarget[i].BlendEnable;

      pDesc->SrcBlend              = m_desc.RenderTarget[0].SrcBlend;
      pDesc->DestBlend             = m_desc.RenderTarget[0].DestBlend;
      pDesc->BlendOp               = m_desc.RenderTarget[0].BlendOp;
      pDesc->SrcBlendAlpha         = m_desc.RenderTarget[0].SrcBlendAlpha;
      pDesc->DestBlendAlpha        = m_desc.RenderTarget[0].DestBlendAlpha;
      pDesc->BlendOpAlpha          = m_desc.RenderTarget[0].BlendOpAlpha;

      pDesc->RenderTargetWriteMask[i] = m_desc.RenderTarget[i].RenderTargetWriteMask;
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10BlendState::GetDesc1(D3D10_BLEND_DESC1* pDesc) {
    *pDesc = m_desc;
  }
  
  
  void D3D10BlendState::BindToContext(
    const Rc<DxvkContext>&  ctx,
          uint32_t          sampleMask) const {
    // We handled Independent Blend during object creation
    // already, so if it is disabled, all elements in the
    // blend mode array will be identical
    for (uint32_t i = 0; i < m_blendModes.size(); i++)
      ctx->setBlendMode(i, m_blendModes.at(i));
    
    // The sample mask is dynamic state in D3D10
    DxvkMultisampleState msState = m_msState;
    msState.sampleMask = sampleMask;
    ctx->setMultisampleState(msState);
    
    // Set up logic op state as well
    ctx->setLogicOpState(m_loState);
  }
  
  
  D3D10_BLEND_DESC1 D3D10BlendState::DefaultDesc() {
    D3D10_BLEND_DESC1 dstDesc;
    dstDesc.AlphaToCoverageEnable  = FALSE;
    dstDesc.IndependentBlendEnable = FALSE;
    
    // 1-7 must be ignored if IndependentBlendEnable is disabled so
    // technically this is not needed, but since this structure is
    // going to be copied around we'll initialize it nonetheless
    for (uint32_t i = 0; i < 8; i++) {
      dstDesc.RenderTarget[i].BlendEnable           = FALSE;
      dstDesc.RenderTarget[i].SrcBlend              = D3D10_BLEND_ONE;
      dstDesc.RenderTarget[i].DestBlend             = D3D10_BLEND_ZERO;
      dstDesc.RenderTarget[i].BlendOp               = D3D10_BLEND_OP_ADD;
      dstDesc.RenderTarget[i].SrcBlendAlpha         = D3D10_BLEND_ONE;
      dstDesc.RenderTarget[i].DestBlendAlpha        = D3D10_BLEND_ZERO;
      dstDesc.RenderTarget[i].BlendOpAlpha          = D3D10_BLEND_OP_ADD;
      dstDesc.RenderTarget[i].RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
    }
    
    return dstDesc;
  }
  
  
  D3D10_BLEND_DESC1 D3D10BlendState::PromoteDesc(const D3D10_BLEND_DESC* pSrcDesc) {
    D3D10_BLEND_DESC1 dstDesc;
    dstDesc.AlphaToCoverageEnable  = pSrcDesc->AlphaToCoverageEnable;
	dstDesc.IndependentBlendEnable = false;
    
    for (uint32_t i = 0; i < 8; i++) {
      dstDesc.RenderTarget[i].BlendEnable           = pSrcDesc->BlendEnable[i];
      dstDesc.RenderTarget[i].SrcBlend              = pSrcDesc->SrcBlend;
      dstDesc.RenderTarget[i].DestBlend             = pSrcDesc->DestBlend;
      dstDesc.RenderTarget[i].BlendOp               = pSrcDesc->BlendOp;
      dstDesc.RenderTarget[i].SrcBlendAlpha         = pSrcDesc->SrcBlendAlpha;
      dstDesc.RenderTarget[i].DestBlendAlpha        = pSrcDesc->DestBlendAlpha;
      dstDesc.RenderTarget[i].BlendOpAlpha          = pSrcDesc->BlendOpAlpha;
      dstDesc.RenderTarget[i].RenderTargetWriteMask = pSrcDesc->RenderTargetWriteMask[i];
    }
    
    return dstDesc;
  }
  
  
  HRESULT D3D10BlendState::NormalizeDesc(D3D10_BLEND_DESC1* pDesc) {
    // TODO validate
    // TODO clear unused values
    return S_OK;
  }
  
  
  DxvkBlendMode D3D10BlendState::DecodeBlendMode(
    const D3D10_RENDER_TARGET_BLEND_DESC1& BlendDesc) {
    DxvkBlendMode mode;
    mode.enableBlending   = BlendDesc.BlendEnable;
    mode.colorSrcFactor   = DecodeBlendFactor(BlendDesc.SrcBlend, false);
    mode.colorDstFactor   = DecodeBlendFactor(BlendDesc.DestBlend, false);
    mode.colorBlendOp     = DecodeBlendOp(BlendDesc.BlendOp);
    mode.alphaSrcFactor   = DecodeBlendFactor(BlendDesc.SrcBlendAlpha, true);
    mode.alphaDstFactor   = DecodeBlendFactor(BlendDesc.DestBlendAlpha, true);
    mode.alphaBlendOp     = DecodeBlendOp(BlendDesc.BlendOpAlpha);
    mode.writeMask        = BlendDesc.RenderTargetWriteMask;
    return mode;
  }
  
  
  VkBlendFactor D3D10BlendState::DecodeBlendFactor(D3D10_BLEND BlendFactor, bool IsAlpha) {
    switch (BlendFactor) {
      case D3D10_BLEND_ZERO:              return VK_BLEND_FACTOR_ZERO;
      case D3D10_BLEND_ONE:               return VK_BLEND_FACTOR_ONE;
      case D3D10_BLEND_SRC_COLOR:         return VK_BLEND_FACTOR_SRC_COLOR;
      case D3D10_BLEND_INV_SRC_COLOR:     return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
      case D3D10_BLEND_SRC_ALPHA:         return VK_BLEND_FACTOR_SRC_ALPHA;
      case D3D10_BLEND_INV_SRC_ALPHA:     return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      case D3D10_BLEND_DEST_ALPHA:        return VK_BLEND_FACTOR_DST_ALPHA;
      case D3D10_BLEND_INV_DEST_ALPHA:    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
      case D3D10_BLEND_DEST_COLOR:        return VK_BLEND_FACTOR_DST_COLOR;
      case D3D10_BLEND_INV_DEST_COLOR:    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
      case D3D10_BLEND_SRC_ALPHA_SAT:     return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
      case D3D10_BLEND_BLEND_FACTOR:      return IsAlpha ? VK_BLEND_FACTOR_CONSTANT_ALPHA : VK_BLEND_FACTOR_CONSTANT_COLOR;
      case D3D10_BLEND_INV_BLEND_FACTOR:  return IsAlpha ? VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
      case D3D10_BLEND_SRC1_COLOR:        return VK_BLEND_FACTOR_SRC1_COLOR;
      case D3D10_BLEND_INV_SRC1_COLOR:    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
      case D3D10_BLEND_SRC1_ALPHA:        return VK_BLEND_FACTOR_SRC1_ALPHA;
      case D3D10_BLEND_INV_SRC1_ALPHA:    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }
    
    if (BlendFactor != 0)  // prevent log spamming when apps use ZeroMemory
      Logger::err(str::format("D3D10: Invalid blend factor: ", BlendFactor));
    return VK_BLEND_FACTOR_ZERO;
  }
  
  
  VkBlendOp D3D10BlendState::DecodeBlendOp(D3D10_BLEND_OP BlendOp) {
    switch (BlendOp) {
      case D3D10_BLEND_OP_ADD:            return VK_BLEND_OP_ADD;
      case D3D10_BLEND_OP_SUBTRACT:       return VK_BLEND_OP_SUBTRACT;
      case D3D10_BLEND_OP_REV_SUBTRACT:   return VK_BLEND_OP_REVERSE_SUBTRACT;
      case D3D10_BLEND_OP_MIN:            return VK_BLEND_OP_MIN;
      case D3D10_BLEND_OP_MAX:            return VK_BLEND_OP_MAX;
    }
    
    if (BlendOp != 0)  // prevent log spamming when apps use ZeroMemory
      Logger::err(str::format("D3D10: Invalid blend op: ", BlendOp));
    return VK_BLEND_OP_ADD;
  }
  
}
