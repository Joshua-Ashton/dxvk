#include "d3d9_stateblock.h"

#include "d3d9_device.h"
#include "d3d9_vertex_declaration.h"
#include "d3d9_buffer.h"
#include "d3d9_shader.h"
#include "d3d9_texture.h"

#include "d3d9_util.h"

namespace dxvk {

  D3D9StateBlock::D3D9StateBlock(D3D9DeviceEx* pDevice, D3D9StateBlockType Type)
    : D3D9StateBlockBase(pDevice)
    , m_deviceState     (pDevice->GetRawState()) {
    CaptureType(Type);
  }


  HRESULT STDMETHODCALLTYPE D3D9StateBlock::QueryInterface(
          REFIID  riid,
          void** ppvObject) {
    if (ppvObject == nullptr)
      return E_POINTER;

    *ppvObject = nullptr;

    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(IDirect3DStateBlock9)) {
      *ppvObject = ref(this);
      return S_OK;
    }

    Logger::warn("D3D9StateBlock::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }


  HRESULT STDMETHODCALLTYPE D3D9StateBlock::Capture() {
    ApplyOrCapture<D3D9StateFunction::Capture>();

    return D3D_OK;
  }


  HRESULT STDMETHODCALLTYPE D3D9StateBlock::Apply() {
    m_applying = true;
    ApplyOrCapture<D3D9StateFunction::Apply>();
    m_applying = false;

    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetVertexDeclaration(D3D9VertexDecl* pDecl) {
    m_state.vertexDecl = pDecl;

    m_captures.flags.set(D3D9CapturedStateFlag::VertexDecl);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetIndices(D3D9IndexBuffer* pIndexData) {
    m_state.indices = pIndexData;

    m_captures.flags.set(D3D9CapturedStateFlag::Indices);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
    m_state.renderStates[State] = Value;

    m_captures.flags.set(D3D9CapturedStateFlag::RenderStates);
    m_captures.renderStates.set<true>(State);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetStateSamplerState(
          DWORD               StateSampler,
          D3DSAMPLERSTATETYPE Type,
          DWORD               Value) {
    m_state.samplerStates[StateSampler][Type] = Value;

    m_captures.flags.set(D3D9CapturedStateFlag::SamplerStates);
    m_captures.samplers.set<true>(StateSampler);
    m_captures.samplerStates[StateSampler].set<true>(Type);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetStreamSource(
          UINT                StreamNumber,
          D3D9VertexBuffer*   pStreamData,
          UINT                OffsetInBytes,
          UINT                Stride) {
    m_state.vertexBuffers[StreamNumber].vertexBuffer = pStreamData;

    m_state.vertexBuffers[StreamNumber].offset = OffsetInBytes;
    m_state.vertexBuffers[StreamNumber].stride = Stride;

    m_captures.flags.set(D3D9CapturedStateFlag::VertexBuffers);
    m_captures.vertexBuffers.set<true>(StreamNumber);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetStreamSourceFreq(UINT StreamNumber, UINT Setting) {
    m_state.streamFreq[StreamNumber] = Setting;

    m_captures.flags.set(D3D9CapturedStateFlag::StreamFreq);
    m_captures.streamFreq.set<true>(StreamNumber);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetStateTexture(DWORD StateSampler, IDirect3DBaseTexture9* pTexture) {
    TextureChangePrivate(m_state.textures[StateSampler], pTexture);

    m_captures.flags.set(D3D9CapturedStateFlag::Textures);
    m_captures.textures.set<true>(StateSampler);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetVertexShader(D3D9VertexShader* pShader) {
    m_state.vertexShader = pShader;

    m_captures.flags.set(D3D9CapturedStateFlag::VertexShader);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetPixelShader(D3D9PixelShader* pShader) {
    m_state.pixelShader = pShader;

    m_captures.flags.set(D3D9CapturedStateFlag::PixelShader);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetMaterial(const D3DMATERIAL9* pMaterial) {
    m_state.material = *pMaterial;

    m_captures.flags.set(D3D9CapturedStateFlag::Material);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetStateTransform(uint32_t idx, const D3DMATRIX* pMatrix) {
    m_state.transforms[idx] = ConvertMatrix(pMatrix);

    m_captures.flags.set(D3D9CapturedStateFlag::Transforms);
    m_captures.transforms.set<true>(idx);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetTextureStageState(
          DWORD                    Stage,
          D3DTEXTURESTAGESTATETYPE Type,
          DWORD                    Value) {
    m_state.textureStages[Stage][Type] = Value;

    m_captures.flags.set(D3D9CapturedStateFlag::TextureStages);
    m_captures.textureStages.set<true>(Stage);
    m_captures.textureStageStates[Stage].set<true>(Type);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::MultiplyStateTransform(uint32_t idx, const D3DMATRIX* pMatrix) {
    m_state.transforms[idx] = ConvertMatrix(pMatrix) * m_state.transforms[idx];

    m_captures.flags.set(D3D9CapturedStateFlag::Transforms);
    m_captures.transforms.set<true>(idx);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetViewport(const D3DVIEWPORT9* pViewport) {
    m_state.viewport = *pViewport;

    m_captures.flags.set(D3D9CapturedStateFlag::Viewport);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetScissorRect(const RECT* pRect) {
    m_state.scissorRect = *pRect;

    m_captures.flags.set(D3D9CapturedStateFlag::ScissorRect);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetClipPlane(DWORD Index, const float* pPlane) {
    for (uint32_t i = 0; i < 4; i++)
      m_state.clipPlanes[Index].coeff[i] = pPlane[i];

    m_captures.flags.set(D3D9CapturedStateFlag::ClipPlanes);
    m_captures.clipPlanes.set<true>(Index);
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetVertexShaderConstantF(
          UINT   StartRegister,
    const float* pConstantData,
          UINT   Vector4fCount) {
    return SetShaderConstants<
      DxsoProgramTypes::VertexShader,
      D3D9ConstantType::Float>(
        StartRegister,
        pConstantData,
        Vector4fCount);
  }


  HRESULT D3D9StateBlock::SetVertexShaderConstantI(
          UINT StartRegister,
    const int* pConstantData,
          UINT Vector4iCount) {
    return SetShaderConstants<
      DxsoProgramTypes::VertexShader,
      D3D9ConstantType::Int>(
        StartRegister,
        pConstantData,
        Vector4iCount);
  }


  HRESULT D3D9StateBlock::SetVertexShaderConstantB(
          UINT  StartRegister,
    const BOOL* pConstantData,
          UINT  BoolCount) {
    return SetShaderConstants<
      DxsoProgramTypes::VertexShader,
      D3D9ConstantType::Bool>(
        StartRegister,
        pConstantData,
        BoolCount);
  }


  HRESULT D3D9StateBlock::SetPixelShaderConstantF(
          UINT   StartRegister,
    const float* pConstantData,
          UINT   Vector4fCount) {
    return SetShaderConstants<
      DxsoProgramTypes::PixelShader,
      D3D9ConstantType::Float>(
        StartRegister,
        pConstantData,
        Vector4fCount);
  }


  HRESULT D3D9StateBlock::SetPixelShaderConstantI(
          UINT StartRegister,
    const int* pConstantData,
          UINT Vector4iCount) {
    return SetShaderConstants<
      DxsoProgramTypes::PixelShader,
      D3D9ConstantType::Int>(
        StartRegister,
        pConstantData,
        Vector4iCount);
  }


  HRESULT D3D9StateBlock::SetPixelShaderConstantB(
          UINT  StartRegister,
    const BOOL* pConstantData,
          UINT  BoolCount) {
    return SetShaderConstants<
      DxsoProgramTypes::PixelShader,
      D3D9ConstantType::Bool>(
        StartRegister,
        pConstantData,
        BoolCount);
  }


  HRESULT D3D9StateBlock::SetVertexBoolBitfield(uint32_t idx, uint32_t mask, uint32_t bits) {
    m_state.vsConsts.bConsts[idx] &= ~mask;
    m_state.vsConsts.bConsts[idx] |= bits & mask;
    return D3D_OK;
  }


  HRESULT D3D9StateBlock::SetPixelBoolBitfield(uint32_t idx, uint32_t mask, uint32_t bits) {
    m_state.psConsts.bConsts[idx] &= ~mask;
    m_state.psConsts.bConsts[idx] |= bits & mask;
    return D3D_OK;
  }


  void D3D9StateBlock::CapturePixelRenderStates() {
    m_captures.flags.set(D3D9CapturedStateFlag::RenderStates);

    m_captures.renderStates.set<true>(D3DRS_ZENABLE);
    m_captures.renderStates.set<true>(D3DRS_FILLMODE);
    m_captures.renderStates.set<true>(D3DRS_SHADEMODE);
    m_captures.renderStates.set<true>(D3DRS_ZWRITEENABLE);
    m_captures.renderStates.set<true>(D3DRS_ALPHATESTENABLE);
    m_captures.renderStates.set<true>(D3DRS_LASTPIXEL);
    m_captures.renderStates.set<true>(D3DRS_SRCBLEND);
    m_captures.renderStates.set<true>(D3DRS_DESTBLEND);
    m_captures.renderStates.set<true>(D3DRS_ZFUNC);
    m_captures.renderStates.set<true>(D3DRS_ALPHAREF);
    m_captures.renderStates.set<true>(D3DRS_ALPHAFUNC);
    m_captures.renderStates.set<true>(D3DRS_DITHERENABLE);
    m_captures.renderStates.set<true>(D3DRS_FOGSTART);
    m_captures.renderStates.set<true>(D3DRS_FOGEND);
    m_captures.renderStates.set<true>(D3DRS_FOGDENSITY);
    m_captures.renderStates.set<true>(D3DRS_ALPHABLENDENABLE);
    m_captures.renderStates.set<true>(D3DRS_DEPTHBIAS);
    m_captures.renderStates.set<true>(D3DRS_STENCILENABLE);
    m_captures.renderStates.set<true>(D3DRS_STENCILFAIL);
    m_captures.renderStates.set<true>(D3DRS_STENCILZFAIL);
    m_captures.renderStates.set<true>(D3DRS_STENCILPASS);
    m_captures.renderStates.set<true>(D3DRS_STENCILFUNC);
    m_captures.renderStates.set<true>(D3DRS_STENCILREF);
    m_captures.renderStates.set<true>(D3DRS_STENCILMASK);
    m_captures.renderStates.set<true>(D3DRS_STENCILWRITEMASK);
    m_captures.renderStates.set<true>(D3DRS_TEXTUREFACTOR);
    m_captures.renderStates.set<true>(D3DRS_WRAP0);
    m_captures.renderStates.set<true>(D3DRS_WRAP1);
    m_captures.renderStates.set<true>(D3DRS_WRAP2);
    m_captures.renderStates.set<true>(D3DRS_WRAP3);
    m_captures.renderStates.set<true>(D3DRS_WRAP4);
    m_captures.renderStates.set<true>(D3DRS_WRAP5);
    m_captures.renderStates.set<true>(D3DRS_WRAP6);
    m_captures.renderStates.set<true>(D3DRS_WRAP7);
    m_captures.renderStates.set<true>(D3DRS_WRAP8);
    m_captures.renderStates.set<true>(D3DRS_WRAP9);
    m_captures.renderStates.set<true>(D3DRS_WRAP10);
    m_captures.renderStates.set<true>(D3DRS_WRAP11);
    m_captures.renderStates.set<true>(D3DRS_WRAP12);
    m_captures.renderStates.set<true>(D3DRS_WRAP13);
    m_captures.renderStates.set<true>(D3DRS_WRAP14);
    m_captures.renderStates.set<true>(D3DRS_WRAP15);
    m_captures.renderStates.set<true>(D3DRS_COLORWRITEENABLE);
    m_captures.renderStates.set<true>(D3DRS_BLENDOP);
    m_captures.renderStates.set<true>(D3DRS_SCISSORTESTENABLE);
    m_captures.renderStates.set<true>(D3DRS_SLOPESCALEDEPTHBIAS);
    m_captures.renderStates.set<true>(D3DRS_ANTIALIASEDLINEENABLE);
    m_captures.renderStates.set<true>(D3DRS_TWOSIDEDSTENCILMODE);
    m_captures.renderStates.set<true>(D3DRS_CCW_STENCILFAIL);
    m_captures.renderStates.set<true>(D3DRS_CCW_STENCILZFAIL);
    m_captures.renderStates.set<true>(D3DRS_CCW_STENCILPASS);
    m_captures.renderStates.set<true>(D3DRS_CCW_STENCILFUNC);
    m_captures.renderStates.set<true>(D3DRS_COLORWRITEENABLE1);
    m_captures.renderStates.set<true>(D3DRS_COLORWRITEENABLE2);
    m_captures.renderStates.set<true>(D3DRS_COLORWRITEENABLE3);
    m_captures.renderStates.set<true>(D3DRS_BLENDFACTOR);
    m_captures.renderStates.set<true>(D3DRS_SRGBWRITEENABLE);
    m_captures.renderStates.set<true>(D3DRS_SEPARATEALPHABLENDENABLE);
    m_captures.renderStates.set<true>(D3DRS_SRCBLENDALPHA);
    m_captures.renderStates.set<true>(D3DRS_DESTBLENDALPHA);
    m_captures.renderStates.set<true>(D3DRS_BLENDOPALPHA);
  }


  void D3D9StateBlock::CapturePixelSamplerStates() {
    m_captures.flags.set(D3D9CapturedStateFlag::SamplerStates);

    for (uint32_t i = 0; i < 17; i++) {
      m_captures.samplers.set<true>(i);

      m_captures.samplerStates[i].set<true>(D3DSAMP_ADDRESSU);
      m_captures.samplerStates[i].set<true>(D3DSAMP_ADDRESSV);
      m_captures.samplerStates[i].set<true>(D3DSAMP_ADDRESSW);
      m_captures.samplerStates[i].set<true>(D3DSAMP_BORDERCOLOR);
      m_captures.samplerStates[i].set<true>(D3DSAMP_MAGFILTER);
      m_captures.samplerStates[i].set<true>(D3DSAMP_MINFILTER);
      m_captures.samplerStates[i].set<true>(D3DSAMP_MIPFILTER);
      m_captures.samplerStates[i].set<true>(D3DSAMP_MIPMAPLODBIAS);
      m_captures.samplerStates[i].set<true>(D3DSAMP_MAXMIPLEVEL);
      m_captures.samplerStates[i].set<true>(D3DSAMP_MAXANISOTROPY);
      m_captures.samplerStates[i].set<true>(D3DSAMP_SRGBTEXTURE);
      m_captures.samplerStates[i].set<true>(D3DSAMP_ELEMENTINDEX);
    }
  }


  void D3D9StateBlock::CapturePixelShaderStates() {
    m_captures.flags.set(D3D9CapturedStateFlag::PixelShader);
    m_captures.flags.set(D3D9CapturedStateFlag::PsConstants);

    m_captures.psConsts.fConsts.setAll();
    m_captures.psConsts.iConsts.setAll();
    m_captures.psConsts.bConsts.setAll();
  }


  void D3D9StateBlock::CaptureVertexRenderStates() {
    m_captures.flags.set(D3D9CapturedStateFlag::RenderStates);

    m_captures.renderStates.set<true>(D3DRS_CULLMODE);
    m_captures.renderStates.set<true>(D3DRS_FOGENABLE);
    m_captures.renderStates.set<true>(D3DRS_FOGCOLOR);
    m_captures.renderStates.set<true>(D3DRS_FOGTABLEMODE);
    m_captures.renderStates.set<true>(D3DRS_FOGSTART);
    m_captures.renderStates.set<true>(D3DRS_FOGEND);
    m_captures.renderStates.set<true>(D3DRS_FOGDENSITY);
    m_captures.renderStates.set<true>(D3DRS_RANGEFOGENABLE);
    m_captures.renderStates.set<true>(D3DRS_AMBIENT);
    m_captures.renderStates.set<true>(D3DRS_COLORVERTEX);
    m_captures.renderStates.set<true>(D3DRS_FOGVERTEXMODE);
    m_captures.renderStates.set<true>(D3DRS_CLIPPING);
    m_captures.renderStates.set<true>(D3DRS_LIGHTING);
    m_captures.renderStates.set<true>(D3DRS_LOCALVIEWER);
    m_captures.renderStates.set<true>(D3DRS_EMISSIVEMATERIALSOURCE);
    m_captures.renderStates.set<true>(D3DRS_AMBIENTMATERIALSOURCE);
    m_captures.renderStates.set<true>(D3DRS_DIFFUSEMATERIALSOURCE);
    m_captures.renderStates.set<true>(D3DRS_SPECULARMATERIALSOURCE);
    m_captures.renderStates.set<true>(D3DRS_VERTEXBLEND);
    m_captures.renderStates.set<true>(D3DRS_CLIPPLANEENABLE);
    m_captures.renderStates.set<true>(D3DRS_POINTSIZE);
    m_captures.renderStates.set<true>(D3DRS_POINTSIZE_MIN);
    m_captures.renderStates.set<true>(D3DRS_POINTSPRITEENABLE);
    m_captures.renderStates.set<true>(D3DRS_POINTSCALEENABLE);
    m_captures.renderStates.set<true>(D3DRS_POINTSCALE_A);
    m_captures.renderStates.set<true>(D3DRS_POINTSCALE_B);
    m_captures.renderStates.set<true>(D3DRS_POINTSCALE_C);
    m_captures.renderStates.set<true>(D3DRS_MULTISAMPLEANTIALIAS);
    m_captures.renderStates.set<true>(D3DRS_MULTISAMPLEMASK);
    m_captures.renderStates.set<true>(D3DRS_PATCHEDGESTYLE);
    m_captures.renderStates.set<true>(D3DRS_POINTSIZE_MAX);
    m_captures.renderStates.set<true>(D3DRS_INDEXEDVERTEXBLENDENABLE);
    m_captures.renderStates.set<true>(D3DRS_TWEENFACTOR);
    m_captures.renderStates.set<true>(D3DRS_POSITIONDEGREE);
    m_captures.renderStates.set<true>(D3DRS_NORMALDEGREE);
    m_captures.renderStates.set<true>(D3DRS_MINTESSELLATIONLEVEL);
    m_captures.renderStates.set<true>(D3DRS_MAXTESSELLATIONLEVEL);
    m_captures.renderStates.set<true>(D3DRS_ADAPTIVETESS_X);
    m_captures.renderStates.set<true>(D3DRS_ADAPTIVETESS_Y);
    m_captures.renderStates.set<true>(D3DRS_ADAPTIVETESS_Z);
    m_captures.renderStates.set<true>(D3DRS_ADAPTIVETESS_W);
    m_captures.renderStates.set<true>(D3DRS_ENABLEADAPTIVETESSELLATION);
    m_captures.renderStates.set<true>(D3DRS_NORMALIZENORMALS);
    m_captures.renderStates.set<true>(D3DRS_SPECULARENABLE);
    m_captures.renderStates.set<true>(D3DRS_SHADEMODE);
  }


  void D3D9StateBlock::CaptureVertexSamplerStates() {
    m_captures.flags.set(D3D9CapturedStateFlag::SamplerStates);

    for (uint32_t i = 17; i < SamplerCount; i++) {
      m_captures.samplers.set<true>(i);
      m_captures.samplerStates[i].set<true>(D3DSAMP_DMAPOFFSET);
    }
  }


  void D3D9StateBlock::CaptureVertexShaderStates() {
    m_captures.flags.set(D3D9CapturedStateFlag::VertexShader);
    m_captures.flags.set(D3D9CapturedStateFlag::VsConstants);

    m_captures.vsConsts.fConsts.setAll();
    m_captures.vsConsts.iConsts.setAll();
    m_captures.vsConsts.bConsts.setAll();
  }


  void D3D9StateBlock::CaptureType(D3D9StateBlockType Type) {
    if (Type == D3D9StateBlockType::PixelState || Type == D3D9StateBlockType::All) {
      CapturePixelRenderStates();
      CapturePixelSamplerStates();
      CapturePixelShaderStates();

      m_captures.flags.set(D3D9CapturedStateFlag::TextureStages);
      m_captures.textureStages.setAll();
      for (auto& stage : m_captures.textureStageStates)
        stage.setAll();
    }

    if (Type == D3D9StateBlockType::VertexState || Type == D3D9StateBlockType::All) {
      CaptureVertexRenderStates();
      CaptureVertexSamplerStates();
      CaptureVertexShaderStates();

      m_captures.flags.set(D3D9CapturedStateFlag::VertexDecl);
      m_captures.flags.set(D3D9CapturedStateFlag::StreamFreq);

      for (uint32_t i = 0; i < caps::MaxStreams; i++)
        m_captures.streamFreq.set<true>(i);
    }

    if (Type == D3D9StateBlockType::All) {
      m_captures.flags.set(D3D9CapturedStateFlag::Textures);
      m_captures.textures.setAll();

      m_captures.flags.set(D3D9CapturedStateFlag::VertexBuffers);
      m_captures.vertexBuffers.setAll();

      m_captures.flags.set(D3D9CapturedStateFlag::Indices);
      m_captures.flags.set(D3D9CapturedStateFlag::Viewport);
      m_captures.flags.set(D3D9CapturedStateFlag::ScissorRect);

      m_captures.flags.set(D3D9CapturedStateFlag::ClipPlanes);
      m_captures.clipPlanes.setAll();

      m_captures.flags.set(D3D9CapturedStateFlag::Transforms);
      m_captures.transforms.setAll();

      m_captures.flags.set(D3D9CapturedStateFlag::Material);
    }

    if (Type != D3D9StateBlockType::None)
      this->Capture();
  }

}