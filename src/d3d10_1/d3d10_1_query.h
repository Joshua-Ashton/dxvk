#pragma once

#include "d3d10_1_device_child.h"

#include "../dxvk/dxvk_event.h"
#include "../dxvk/dxvk_query.h"

namespace dxvk {
  
  class D3D10Query : public D3D10DeviceChild<ID3D10Predicate> {
    
  public:
    
    D3D10Query(
            D3D10Device*      device,
      const D3D10_QUERY_DESC& desc);
    
    ~D3D10Query();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID  riid,
            void**  ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(
            ID3D10Device **ppDevice) final;
    
    UINT STDMETHODCALLTYPE GetDataSize();
    
    void STDMETHODCALLTYPE GetDesc(
            D3D10_QUERY_DESC *pDesc) final;
    
    uint32_t Reset();
    
    bool HasBeginEnabled() const;
    
    void Begin(DxvkContext* ctx, uint32_t revision);
    
    void End(DxvkContext* ctx);
    
    void Signal(DxvkContext* ctx, uint32_t revision);
    
    HRESULT STDMETHODCALLTYPE GetData(
            void*                             pData,
			UINT							  DataSize,
            UINT                              GetDataFlags);

	void STDMETHODCALLTYPE Begin(void);

	void STDMETHODCALLTYPE End(void);
    
  private:
    
    D3D10Device* const m_device;
    D3D10_QUERY_DESC   m_desc;
    
    Rc<DxvkQuery> m_query = nullptr;
    Rc<DxvkEvent> m_event = nullptr;
    
    uint32_t m_revision = 0;
    
  };
  
}
