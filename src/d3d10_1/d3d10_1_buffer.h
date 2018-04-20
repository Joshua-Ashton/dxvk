#pragma once

#include <dxvk_device.h>

#include "d3d10_1_device_child.h"
#include "d3d10_1_interfaces.h"

namespace dxvk {
  
  class D3D10Device;
  class D3D10DeviceContext;
  
  
  /**
   * \brief Common buffer info
   * 
   * Stores where the buffer was last
   * mapped on the immediate context.
   */
  struct D3D10BufferInfo {
    DxvkPhysicalBufferSlice mappedSlice;
  };
  
  
  class D3D10Buffer : public D3D10DeviceChild<ID3D10Buffer> {
    static constexpr VkDeviceSize BufferSliceAlignment = 64;
  public:
    
    D3D10Buffer(
            D3D10Device*                pDevice,
      const D3D10_BUFFER_DESC*          pDesc);
    ~D3D10Buffer();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID  riid,
            void**  ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(
            ID3D10Device **ppDevice) final;
    
    void STDMETHODCALLTYPE GetType(
            D3D10_RESOURCE_DIMENSION *pResourceDimension) final;
    
    UINT STDMETHODCALLTYPE GetEvictionPriority() final;
    
    void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) final;
    
    void STDMETHODCALLTYPE GetDesc(
            D3D10_BUFFER_DESC *pDesc) final;
    
    Rc<DxvkBuffer> GetBuffer() const {
      return m_buffer;
    }
    
    DxvkBufferSlice GetBufferSlice() const {
      return DxvkBufferSlice(m_buffer, 0, m_buffer->info().size);
    }
    
    DxvkBufferSlice GetBufferSlice(VkDeviceSize offset) const {
      return DxvkBufferSlice(m_buffer, offset, m_buffer->info().size - offset);
    }
    
    DxvkBufferSlice GetBufferSlice(VkDeviceSize offset, VkDeviceSize length) const {
      return DxvkBufferSlice(m_buffer, offset, length);
    }
    
    VkDeviceSize GetSize() const {
      return m_buffer->info().size;
    }
    
    D3D10BufferInfo* GetBufferInfo() {
      return &m_bufferInfo;
    }

    HRESULT STDMETHODCALLTYPE Map( 
          D3D10_MAP MapType,
          UINT MapFlags,
          void **ppData) final;
    
    void STDMETHODCALLTYPE Unmap(void) final;
    
  private:
    
    const Com<D3D10Device>      m_device;
    const D3D10_BUFFER_DESC     m_desc;
    
    Rc<DxvkBuffer>              m_buffer;
    D3D10BufferInfo             m_bufferInfo;
    
    Rc<DxvkBuffer> CreateBuffer(
      const D3D10_BUFFER_DESC* pDesc) const;
    
    VkMemoryPropertyFlags GetMemoryFlags(
      const D3D10_BUFFER_DESC* pDesc) const;
    
  };
  
}
