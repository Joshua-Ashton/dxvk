#pragma once

#include "d3d10_1_context.h"

namespace dxvk {
  
  class D3D10CommandList : public D3D10DeviceChild<ID3D10CommandList> {
    
  public:
    
    D3D10CommandList(
            D3D10Device*  pDevice,
            UINT          ContextFlags);
    
    ~D3D10CommandList();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID  riid,
            void**  ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(
            ID3D10Device **ppDevice) final;
    
    UINT STDMETHODCALLTYPE GetContextFlags() final;
    
    void AddChunk(
            Rc<DxvkCsChunk>&&   Chunk,
            UINT                DrawCount);
    
    void EmitToCommandList(
            ID3D10CommandList*  pCommandList);
    
    void EmitToCsThread(
            DxvkCsThread*       CsThread);
    
    UINT GetDrawCount() const {
      return m_drawCount;
    }
    
  private:
    
    D3D10Device* const m_device;
    UINT         const m_contextFlags;
    UINT               m_drawCount = 0;
    
    std::vector<Rc<DxvkCsChunk>> m_chunks;
    
  };
  
}