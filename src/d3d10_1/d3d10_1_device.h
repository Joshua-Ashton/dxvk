#pragma once

#include <mutex>
#include <vector>

#include "../dxbc/dxbc_options.h"

#include "../dxgi/dxgi_object.h"

#include "../util/com/com_private_data.h"

#include "d3d10_1_interfaces.h"
#include "d3d10_1_options.h"
#include "d3d10_1_shader.h"
#include "d3d10_1_state.h"
#include "d3d10_1_util.h"
#include "d3d10_1_context_state.h"

#include "../dxvk/dxvk_cs.h"

namespace dxvk {
  class DxgiAdapter;
  
  class D3D10Buffer;
  class D3D10Counter;
  class D3D10Predicate;
  class D3D10Presenter;
  class D3D10Query;
  class D3D10ShaderModule;
  class D3D10Texture1D;
  class D3D10Texture2D;
  class D3D10Texture3D;
  
  /**
   * \brief D3D10 device container
   * 
   * Stores all the objects that contribute to the D3D10
   * device implementation, including the DXGI device.
   */
  class D3D10DeviceContainer : public DxgiObject<IDXGIObject> {
    
  public:
    
    D3D10DeviceContainer();
    ~D3D10DeviceContainer();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID                  riid,
            void**                  ppvObject);
    
    HRESULT STDMETHODCALLTYPE GetParent(
            REFIID                  riid,
            void**                  ppParent);
    
    IDXGIVkDevice*  m_dxgiDevice      = nullptr;
    D3D10Device*    m_d3d10Device     = nullptr;
    D3D10Presenter* m_d3d10Presenter  = nullptr;
    
  };
  
  
  /**
   * \brief D3D10 device implementation
   * 
   * Implements the ID3D10Device interfaces
   * as part of a \ref D3D10DeviceContainer.
   */
  class D3D10Device final : public ID3D10Device1 {
    /// Maximum number of resource init commands per command buffer
    constexpr static uint64_t InitCommandThreshold = 50;
  public:
    
    D3D10Device(
            IDXGIObject*            pContainer,
            IDXGIVkDevice*          pDxgiDevice,
            D3D10_FEATURE_LEVEL1    FeatureLevel,
            UINT                    FeatureFlags);
    ~D3D10Device();
    
    ULONG STDMETHODCALLTYPE AddRef();
    
    ULONG STDMETHODCALLTYPE Release();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID                  riid,
            void**                  ppvObject);
    
    HRESULT STDMETHODCALLTYPE CreateBuffer(
      const D3D10_BUFFER_DESC*      pDesc,
      const D3D10_SUBRESOURCE_DATA* pInitialData,
            ID3D10Buffer**          ppBuffer);
    
    HRESULT STDMETHODCALLTYPE CreateTexture1D(
      const D3D10_TEXTURE1D_DESC*   pDesc,
      const D3D10_SUBRESOURCE_DATA* pInitialData,
            ID3D10Texture1D**       ppTexture1D);
    
    HRESULT STDMETHODCALLTYPE CreateTexture2D(
      const D3D10_TEXTURE2D_DESC*   pDesc,
      const D3D10_SUBRESOURCE_DATA* pInitialData,
            ID3D10Texture2D**       ppTexture2D);
    
    HRESULT STDMETHODCALLTYPE CreateTexture3D(
      const D3D10_TEXTURE3D_DESC*   pDesc,
      const D3D10_SUBRESOURCE_DATA* pInitialData,
            ID3D10Texture3D**       ppTexture3D);
    
    HRESULT STDMETHODCALLTYPE CreateShaderResourceView(
            ID3D10Resource*                   pResource,
      const D3D10_SHADER_RESOURCE_VIEW_DESC*  pDesc,
            ID3D10ShaderResourceView**        ppSRView);

	HRESULT STDMETHODCALLTYPE CreateShaderResourceView1(
            ID3D10Resource*                   pResource,
      const D3D10_SHADER_RESOURCE_VIEW_DESC1*  pDesc,
            ID3D10ShaderResourceView1**        ppSRView);
    
    HRESULT STDMETHODCALLTYPE CreateRenderTargetView(
            ID3D10Resource*                   pResource,
      const D3D10_RENDER_TARGET_VIEW_DESC*    pDesc,
            ID3D10RenderTargetView**          ppRTView);
    
    HRESULT STDMETHODCALLTYPE CreateDepthStencilView(
            ID3D10Resource*                   pResource,
      const D3D10_DEPTH_STENCIL_VIEW_DESC*    pDesc,
            ID3D10DepthStencilView**          ppDepthStencilView);
    
    HRESULT STDMETHODCALLTYPE CreateInputLayout(
      const D3D10_INPUT_ELEMENT_DESC*   pInputElementDescs,
            UINT                        NumElements,
      const void*                       pShaderBytecodeWithInputSignature,
            SIZE_T                      BytecodeLength,
            ID3D10InputLayout**         ppInputLayout);
    
    HRESULT STDMETHODCALLTYPE CreateVertexShader(
      const void*                       pShaderBytecode,
            SIZE_T                      BytecodeLength,

            ID3D10VertexShader**        ppVertexShader);
    
    HRESULT STDMETHODCALLTYPE CreateGeometryShader(
      const void*                       pShaderBytecode,
            SIZE_T                      BytecodeLength,

            ID3D10GeometryShader**      ppGeometryShader);
    
    HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(
      const void*                       pShaderBytecode,
            SIZE_T                      BytecodeLength,
      const D3D10_SO_DECLARATION_ENTRY* pSODeclaration,
            UINT                        NumEntries,
			UINT						OutputStreamStride,

            ID3D10GeometryShader**      ppGeometryShader);
    
    HRESULT STDMETHODCALLTYPE CreatePixelShader(
      const void*                       pShaderBytecode,
            SIZE_T                      BytecodeLength,

            ID3D10PixelShader**         ppPixelShader);
    
    HRESULT STDMETHODCALLTYPE CreateBlendState(
      const D3D10_BLEND_DESC*           pBlendStateDesc,
            ID3D10BlendState**          ppBlendState);
    
    HRESULT STDMETHODCALLTYPE CreateBlendState1(
      const D3D10_BLEND_DESC1*          pBlendStateDesc,
            ID3D10BlendState1**         ppBlendState);

    HRESULT STDMETHODCALLTYPE CreateDepthStencilState(
      const D3D10_DEPTH_STENCIL_DESC*   pDepthStencilDesc,
            ID3D10DepthStencilState**   ppDepthStencilState);
    
    HRESULT STDMETHODCALLTYPE CreateRasterizerState(
      const D3D10_RASTERIZER_DESC*      pRasterizerDesc,
            ID3D10RasterizerState**     ppRasterizerState);
    
    HRESULT STDMETHODCALLTYPE CreateSamplerState(
      const D3D10_SAMPLER_DESC*         pSamplerDesc,
            ID3D10SamplerState**        ppSamplerState);
    
    HRESULT STDMETHODCALLTYPE CreateQuery(
      const D3D10_QUERY_DESC*           pQueryDesc,
            ID3D10Query**               ppQuery);
    
    HRESULT STDMETHODCALLTYPE CreatePredicate(
      const D3D10_QUERY_DESC*           pPredicateDesc,
            ID3D10Predicate**           ppPredicate);
    
    HRESULT STDMETHODCALLTYPE CreateCounter(
      const D3D10_COUNTER_DESC*         pCounterDesc,
            ID3D10Counter**             ppCounter);
    
    HRESULT STDMETHODCALLTYPE OpenSharedResource(
            HANDLE      hResource,
            REFIID      ReturnedInterface,
            void**      ppResource);

    HRESULT STDMETHODCALLTYPE OpenSharedResource1(
            HANDLE      hResource,
            REFIID      returnedInterface,
            void**      ppResource);

    HRESULT STDMETHODCALLTYPE OpenSharedResourceByName(
            LPCWSTR     lpName,
            DWORD       dwDesiredAccess,
            REFIID      returnedInterface,
            void**      ppResource);
    
    HRESULT STDMETHODCALLTYPE CheckFormatSupport(
            DXGI_FORMAT Format,
            UINT*       pFormatSupport);
    
    HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels(
            DXGI_FORMAT Format,
            UINT        SampleCount,
            UINT*       pNumQualityLevels);
    
    void STDMETHODCALLTYPE CheckCounterInfo(
            D3D10_COUNTER_INFO* pCounterInfo);
    
    HRESULT STDMETHODCALLTYPE CheckCounter(
      const D3D10_COUNTER_DESC* pDesc,
            D3D10_COUNTER_TYPE* pType,
            UINT*               pActiveCounters,
            LPSTR               szName,
            UINT*               pNameLength,
            LPSTR               szUnits,
            UINT*               pUnitsLength,
            LPSTR               szDescription,
            UINT*               pDescriptionLength);
    
    HRESULT STDMETHODCALLTYPE GetPrivateData(
            REFGUID Name,
            UINT    *pDataSize,
            void    *pData);
    
    HRESULT STDMETHODCALLTYPE SetPrivateData(
            REFGUID Name,
            UINT    DataSize,
      const void    *pData);
    
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
            REFGUID  Name,
      const IUnknown *pUnknown);
    
    D3D10_FEATURE_LEVEL1 STDMETHODCALLTYPE GetFeatureLevel();
    
    UINT STDMETHODCALLTYPE GetCreationFlags();
    
    HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason();
    
    HRESULT STDMETHODCALLTYPE SetExceptionMode(UINT RaiseFlags);
    
    UINT STDMETHODCALLTYPE GetExceptionMode();
    
    Rc<DxvkDevice> GetDXVKDevice() {
      return m_dxvkDevice;
    }
    
    void FlushInitContext();
    
    VkPipelineStageFlags GetEnabledShaderStages() const;
    
    DXGI_VK_FORMAT_INFO STDMETHODCALLTYPE LookupFormat(
            DXGI_FORMAT           format,
            DXGI_VK_FORMAT_MODE   mode) const;
    
    bool TestOption(D3D10Option Option) const {
      return m_d3d10Options.test(Option);
    }
    
    static bool CheckFeatureLevelSupport(
      const Rc<DxvkAdapter>&  adapter,
            D3D10_FEATURE_LEVEL1 featureLevel);
    
    static VkPhysicalDeviceFeatures GetDeviceFeatures(
      const Rc<DxvkAdapter>&  adapter,
            D3D10_FEATURE_LEVEL1 featureLevel);

	// CONTEXT CODE

        void STDMETHODCALLTYPE VSSetConstantBuffers(UINT, UINT, ID3D10Buffer *const *);
        void STDMETHODCALLTYPE PSSetShaderResources(UINT, UINT, ID3D10ShaderResourceView *const *);
        void STDMETHODCALLTYPE PSSetShader(ID3D10PixelShader *);
        void STDMETHODCALLTYPE PSSetSamplers(UINT, UINT, ID3D10SamplerState *const *);
        void STDMETHODCALLTYPE VSSetShader(ID3D10VertexShader *);
        void STDMETHODCALLTYPE DrawIndexed(UINT, UINT, INT);
        void STDMETHODCALLTYPE Draw(UINT, UINT);
        void STDMETHODCALLTYPE PSSetConstantBuffers(UINT, UINT, ID3D10Buffer *const *);
        void STDMETHODCALLTYPE IASetInputLayout(ID3D10InputLayout *);
        void STDMETHODCALLTYPE IASetVertexBuffers(UINT, UINT, ID3D10Buffer *const *, const UINT *, const UINT *);
        void STDMETHODCALLTYPE IASetIndexBuffer(ID3D10Buffer *, DXGI_FORMAT, UINT);
        void STDMETHODCALLTYPE DrawIndexedInstanced(UINT, UINT, UINT, INT, UINT);
        void STDMETHODCALLTYPE DrawInstanced(UINT, UINT, UINT, UINT);
        void STDMETHODCALLTYPE GSSetConstantBuffers(UINT, UINT, ID3D10Buffer *const *);
        void STDMETHODCALLTYPE GSSetShader(ID3D10GeometryShader *);
        void STDMETHODCALLTYPE IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY);
        void STDMETHODCALLTYPE VSSetShaderResources(UINT, UINT, ID3D10ShaderResourceView *const *);
        void STDMETHODCALLTYPE VSSetSamplers(UINT, UINT, ID3D10SamplerState *const *);
        void STDMETHODCALLTYPE SetPredication(ID3D10Predicate *, BOOL);
        void STDMETHODCALLTYPE GSSetShaderResources(UINT, UINT, ID3D10ShaderResourceView *const *);
        void STDMETHODCALLTYPE GSSetSamplers(UINT, UINT, ID3D10SamplerState *const *);
        void STDMETHODCALLTYPE OMSetRenderTargets(UINT, ID3D10RenderTargetView *const *, ID3D10DepthStencilView *);
        void STDMETHODCALLTYPE OMSetBlendState(ID3D10BlendState *, const FLOAT[], UINT);
        void STDMETHODCALLTYPE OMSetDepthStencilState(ID3D10DepthStencilState *, UINT);
        void STDMETHODCALLTYPE SOSetTargets(UINT, ID3D10Buffer *const *, const UINT *);
        void STDMETHODCALLTYPE DrawAuto(void);
        void STDMETHODCALLTYPE RSSetState(ID3D10RasterizerState *);
        void STDMETHODCALLTYPE RSSetViewports(UINT, const D3D10_VIEWPORT *);
        void STDMETHODCALLTYPE RSSetScissorRects(UINT, const D3D10_RECT *);
        void STDMETHODCALLTYPE CopySubresourceRegion(ID3D10Resource *, UINT, UINT, UINT, UINT, ID3D10Resource *, UINT, const D3D10_BOX *);
        void STDMETHODCALLTYPE CopyResource(ID3D10Resource *, ID3D10Resource *);
        void STDMETHODCALLTYPE UpdateSubresource(ID3D10Resource *, UINT, const D3D10_BOX *, const void *, UINT, UINT);
        void STDMETHODCALLTYPE ClearRenderTargetView(ID3D10RenderTargetView *, const FLOAT[]);
        void STDMETHODCALLTYPE ClearDepthStencilView(ID3D10DepthStencilView *, UINT, FLOAT, UINT8);
        void STDMETHODCALLTYPE GenerateMips(ID3D10ShaderResourceView *);
        void STDMETHODCALLTYPE ResolveSubresource(ID3D10Resource *, UINT, ID3D10Resource *, UINT, DXGI_FORMAT);
        void STDMETHODCALLTYPE VSGetConstantBuffers(UINT, UINT, ID3D10Buffer **);
        void STDMETHODCALLTYPE PSGetShaderResources(UINT, UINT, ID3D10ShaderResourceView **);
        void STDMETHODCALLTYPE PSGetShader(ID3D10PixelShader **);
        void STDMETHODCALLTYPE PSGetSamplers(UINT, UINT, ID3D10SamplerState **);
        void STDMETHODCALLTYPE VSGetShader(ID3D10VertexShader **);
        void STDMETHODCALLTYPE PSGetConstantBuffers(UINT, UINT, ID3D10Buffer **);
        void STDMETHODCALLTYPE IAGetInputLayout(ID3D10InputLayout **);
        void STDMETHODCALLTYPE IAGetVertexBuffers(UINT, UINT, ID3D10Buffer **, UINT *, UINT *);
        void STDMETHODCALLTYPE IAGetIndexBuffer(ID3D10Buffer **, DXGI_FORMAT *, UINT *);
        void STDMETHODCALLTYPE GSGetConstantBuffers(UINT, UINT, ID3D10Buffer **);
        void STDMETHODCALLTYPE GSGetShader(ID3D10GeometryShader **);
        void STDMETHODCALLTYPE IAGetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY *);
        void STDMETHODCALLTYPE VSGetShaderResources(UINT, UINT, ID3D10ShaderResourceView **);
        void STDMETHODCALLTYPE VSGetSamplers(UINT, UINT, ID3D10SamplerState **);
        void STDMETHODCALLTYPE GetPredication(ID3D10Predicate **, BOOL *);
        void STDMETHODCALLTYPE GSGetShaderResources(UINT, UINT, ID3D10ShaderResourceView **);
        void STDMETHODCALLTYPE GSGetSamplers(UINT, UINT, ID3D10SamplerState **);
        void STDMETHODCALLTYPE OMGetRenderTargets(UINT, ID3D10RenderTargetView **, ID3D10DepthStencilView **);
        void STDMETHODCALLTYPE OMGetBlendState(ID3D10BlendState **, FLOAT[], UINT *);
        void STDMETHODCALLTYPE OMGetDepthStencilState(ID3D10DepthStencilState **, UINT *);
        void STDMETHODCALLTYPE SOGetTargets(UINT, ID3D10Buffer **, UINT *);
        void STDMETHODCALLTYPE RSGetState(ID3D10RasterizerState **);
        void STDMETHODCALLTYPE RSGetViewports(UINT *, D3D10_VIEWPORT *);
        void STDMETHODCALLTYPE RSGetScissorRects(UINT *, D3D10_RECT *);
        void STDMETHODCALLTYPE ClearState(void);
        void STDMETHODCALLTYPE Flush(void);
        void STDMETHODCALLTYPE SetTextFilterSize(UINT, UINT);
        void STDMETHODCALLTYPE GetTextFilterSize(UINT *, UINT *);
    
    
    template<typename Cmd>
    void EmitCs(Cmd&& command) {
      if (!m_csChunk->push(command)) {
        EmitCsChunk(std::move(m_csChunk));
        
        m_csChunk = new DxvkCsChunk();
        m_csChunk->push(command);
      }
    }
    
    void FlushCsChunk() {
      if (m_csChunk->commandCount() != 0) {
        EmitCsChunk(std::move(m_csChunk));
        m_csChunk = new DxvkCsChunk();
      }
    }
    
    void EmitCsChunk(Rc<DxvkCsChunk>&& chunk);

    bool WaitForResource(
      const Rc<DxvkResource>&                 Resource,
            UINT                              MapFlags);

    void SynchronizeCsThread(); 

  private:
    // Context & State
	Rc<DxvkCsChunk>             m_csChunk;
	Rc<DxvkDataBuffer>          m_updateBuffer;

    Com<D3D10BlendState>        m_defaultBlendState;
    Com<D3D10DepthStencilState> m_defaultDepthStencilState;
    Com<D3D10RasterizerState>   m_defaultRasterizerState;

    D3D10ContextState           m_state;
    UINT                        m_drawCount = 0;

	DxvkCsThread* m_csThread;
	constexpr static UINT MaxPendingDraws = 500;
	bool         m_csIsBusy = false;

    void ApplyInputLayout();
    
    void ApplyPrimitiveTopology();
    
    void ApplyBlendState();
    
    void ApplyBlendFactor();
    
    void ApplyDepthStencilState();
    
    void ApplyStencilRef();
    
    void ApplyRasterizerState();
    
    void ApplyViewportState();
    
    void BindFramebuffer();

    template<typename T>
    void BindShader(
            T*                                pShader,
            VkShaderStageFlagBits             Stage) {
      EmitCs([
        cShader = pShader != nullptr ? pShader->GetShader() : nullptr,
        cStage  = Stage
      ] (DxvkContext* ctx) {
        ctx->bindShader(cStage, cShader);
      });
    }
    
    void BindVertexBuffer(
            UINT                              Slot,
            D3D10Buffer*                      pBuffer,
            UINT                              Offset,
            UINT                              Stride);
    
    void BindIndexBuffer(
            D3D10Buffer*                      pBuffer,
            UINT                              Offset,
            DXGI_FORMAT                       Format);
    
    void BindConstantBuffer(
            UINT                              Slot,
      const D3D10ConstantBufferBinding*       pBufferBinding);
    
    void BindSampler(
            UINT                              Slot,
            D3D10SamplerState*                pSampler);
    
    void BindShaderResource(
            UINT                              Slot,
            D3D10ShaderResourceView*          pResource);
    
    void SetConstantBuffers(
            DxbcProgramType                   ShaderStage,
            D3D10ConstantBufferBindings&      Bindings,
            UINT                              StartSlot,
            UINT                              NumBuffers,
            ID3D10Buffer* const*              ppConstantBuffers,
      const UINT*                             pFirstConstant,
      const UINT*                             pNumConstants);
    
    void SetSamplers(
            DxbcProgramType                   ShaderStage,
            D3D10SamplerBindings&             Bindings,
            UINT                              StartSlot,
            UINT                              NumSamplers,
            ID3D10SamplerState* const*        ppSamplers);
    
    void SetShaderResources(
            DxbcProgramType                   ShaderStage,
            D3D10ShaderResourceBindings&      Bindings,
            UINT                              StartSlot,
            UINT                              NumResources,
            ID3D10ShaderResourceView* const*  ppResources);
    
    void GetConstantBuffers(
      const D3D10ConstantBufferBindings&      Bindings,
            UINT                              StartSlot,
            UINT                              NumBuffers,
            ID3D10Buffer**                    ppConstantBuffers, 
            UINT*                             pFirstConstant, 
            UINT*                             pNumConstants);
    
    void RestoreState();
    
    void RestoreConstantBuffers(
            DxbcProgramType                   Stage,
            D3D10ConstantBufferBindings&      Bindings);
    
    void RestoreSamplers(
            DxbcProgramType                   Stage,
            D3D10SamplerBindings&             Bindings);
    
    void RestoreShaderResources(
            DxbcProgramType                   Stage,
            D3D10ShaderResourceBindings&      Bindings);
    
    DxvkDataSlice AllocUpdateBufferSlice(size_t Size);

    ///
    
    IDXGIObject*                    m_container;
    Com<IDXGIVkAdapter>             m_dxgiAdapter;
    
    const D3D10_FEATURE_LEVEL1      m_featureLevel;
    const UINT                      m_featureFlags;
    
    const Rc<DxvkDevice>            m_dxvkDevice;
    const Rc<DxvkAdapter>           m_dxvkAdapter;
    
    const D3D10OptionSet            m_d3d10Options;
    const DxbcOptions               m_dxbcOptions;
    
    std::mutex                      m_resourceInitMutex;
    Rc<DxvkContext>                 m_resourceInitContext;
    uint64_t                        m_resourceInitCommands = 0;
    
    D3D10StateObjectSet<D3D10BlendState>        m_bsStateObjects;
    D3D10StateObjectSet<D3D10DepthStencilState> m_dsStateObjects;
    D3D10StateObjectSet<D3D10RasterizerState>   m_rsStateObjects;
    D3D10StateObjectSet<D3D10SamplerState>      m_samplerObjects;
    D3D10ShaderModuleSet                        m_shaderModules;
    
    HRESULT CreateShaderModule(
            D3D10ShaderModule*      pShaderModule,
      const void*                   pShaderBytecode,
            size_t                  BytecodeLength,
            DxbcProgramType         ProgramType);
    
    void InitBuffer(
            D3D10Buffer*                pBuffer,
      const D3D10_SUBRESOURCE_DATA*     pInitialData);
    
    void InitTexture(
      const Rc<DxvkImage>&              image,
      const D3D10_SUBRESOURCE_DATA*     pInitialData);
    
    HRESULT GetFormatSupportFlags(
            DXGI_FORMAT Format,
            UINT*       pFlags1,
            UINT*       pFlags2) const;
    
    BOOL GetImageTypeSupport(
            VkFormat    Format,
            VkImageType Type) const;
    
    void LockResourceInitContext();
    void UnlockResourceInitContext(uint64_t CommandCount);
    void SubmitResourceInitCommands();
    
    static D3D10_FEATURE_LEVEL1 GetMaxFeatureLevel();
    
  };
  
}
