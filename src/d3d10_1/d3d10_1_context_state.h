#pragma once

#include <array>

#include "d3d10_1_buffer.h"
#include "d3d10_1_input_layout.h"
#include "d3d10_1_query.h"
#include "d3d10_1_sampler.h"
#include "d3d10_1_shader.h"
#include "d3d10_1_state.h"
#include "d3d10_1_view_dsv.h"
#include "d3d10_1_view_rtv.h"
#include "d3d10_1_view_srv.h"

namespace dxvk {
  
  struct D3D10ConstantBufferBinding {
    Com<D3D10Buffer> buffer         = nullptr;
    UINT             constantOffset = 0;
    UINT             constantCount  = 0;
  };
  
  using D3D10ConstantBufferBindings = std::array<
    D3D10ConstantBufferBinding, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT>;
  
  
  using D3D10SamplerBindings = std::array<
    Com<D3D10SamplerState>, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT>;
    
  
  using D3D10ShaderResourceBindings = std::array<
    Com<D3D10ShaderResourceView>, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>;
  
  
  struct D3D10ContextStateVS {
    Com<D3D10VertexShader>        shader;
    D3D10ConstantBufferBindings   constantBuffers;
    D3D10SamplerBindings          samplers;
    D3D10ShaderResourceBindings   shaderResources;
  };
  
  
  struct D3D10ContextStateGS {
    Com<D3D10GeometryShader>      shader;
    D3D10ConstantBufferBindings   constantBuffers;
    D3D10SamplerBindings          samplers;
    D3D10ShaderResourceBindings   shaderResources;
  };
  
  
  struct D3D10ContextStatePS {
    Com<D3D10PixelShader>         shader;
    D3D10ConstantBufferBindings   constantBuffers;
    D3D10SamplerBindings          samplers;
    D3D10ShaderResourceBindings   shaderResources;
  };
  
  
  
  struct D3D10VertexBufferBinding {
    Com<D3D10Buffer> buffer = nullptr;
    UINT             offset = 0;
    UINT             stride = 0;
  };
  
  
  struct D3D10IndexBufferBinding {
    Com<D3D10Buffer> buffer = nullptr;
    UINT             offset = 0;
    DXGI_FORMAT      format = DXGI_FORMAT_UNKNOWN;
  };
  
  
  struct D3D10ContextStateIA {
    Com<D3D10InputLayout>    inputLayout;
    D3D10_PRIMITIVE_TOPOLOGY primitiveTopology = D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;
    
    std::array<D3D10VertexBufferBinding, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> vertexBuffers;
    D3D10IndexBufferBinding                                                         indexBuffer;
  };
  
  
  struct D3D10ContextStateOM {
    std::array<Com<D3D10RenderTargetView>, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT> renderTargetViews;
    Com<D3D10DepthStencilView>                                                     depthStencilView;
    
    Com<D3D10BlendState>        cbState = nullptr;
    Com<D3D10DepthStencilState> dsState = nullptr;
    
    FLOAT blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    UINT  sampleMask     = 0xFFFFFFFFu;
    UINT  stencilRef     = 0u;
  };
  
  
  struct D3D10ContextStateRS {
    uint32_t numViewports = 0;
    uint32_t numScissors  = 0;
    
    std::array<D3D10_VIEWPORT, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> viewports;
    std::array<D3D10_RECT,     D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> scissors;
    
    Com<D3D10RasterizerState> state;
  };
  
  
  struct D3D10ContextStateSO {
    std::array<Com<D3D10Buffer>, 4> targets;
  };
  
  
  struct D3D10ContextStatePR {
    Com<D3D10Query> predicateObject = nullptr;
    BOOL            predicateValue  = FALSE;
  };
  
  
  /**
   * \brief Context state
   */
  struct D3D10ContextState {
    D3D10ContextStateGS gs;
    D3D10ContextStatePS ps;
    D3D10ContextStateVS vs;
    
    D3D10ContextStateIA ia;
    D3D10ContextStateOM om;
    D3D10ContextStateRS rs;
    D3D10ContextStateSO so;
    D3D10ContextStatePR pr;
  };
  
}