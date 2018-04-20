#include "d3d10_1_buffer.h"
#include "d3d10_1_device.h"

#include "../dxvk/dxvk_data.h"

namespace dxvk {
  
  D3D10Buffer::D3D10Buffer(
          D3D10Device*                pDevice,
    const D3D10_BUFFER_DESC*          pDesc)
  : m_device    (pDevice),
    m_desc      (*pDesc),
    m_buffer    (CreateBuffer(pDesc)),
    m_bufferInfo{ m_buffer->slice() } {
    
  }
  
  
  D3D10Buffer::~D3D10Buffer() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Buffer::QueryInterface(REFIID riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(ID3D10DeviceChild)
     || riid == __uuidof(ID3D10Resource)
     || riid == __uuidof(ID3D10Buffer)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    Logger::warn("D3D10Buffer::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  
  
  void STDMETHODCALLTYPE D3D10Buffer::GetDevice(ID3D10Device** ppDevice) {
    *ppDevice = m_device.ref();
  }
  
  
  UINT STDMETHODCALLTYPE D3D10Buffer::GetEvictionPriority() {
    Logger::warn("D3D10Buffer::GetEvictionPriority: Stub");
    return DXGI_RESOURCE_PRIORITY_NORMAL;
  }
  
  
  void STDMETHODCALLTYPE D3D10Buffer::SetEvictionPriority(UINT EvictionPriority) {
    Logger::warn("D3D10Buffer::SetEvictionPriority: Stub");
  }
  
  
  void STDMETHODCALLTYPE D3D10Buffer::GetType(D3D10_RESOURCE_DIMENSION* pResourceDimension) {
    *pResourceDimension = D3D10_RESOURCE_DIMENSION_BUFFER;
  }
  
  
  void STDMETHODCALLTYPE D3D10Buffer::GetDesc(D3D10_BUFFER_DESC* pDesc) {
    *pDesc = m_desc;
  }
  
  HRESULT STDMETHODCALLTYPE D3D10Buffer::Map(
      D3D10_MAP MapType,
      UINT MapFlags,
      void **ppData)
  {
	  Rc<DxvkBuffer> buffer = GetBuffer();

	  if (!(buffer->memFlags() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
		  Logger::err("D3D11: Cannot map a device-local buffer");
		  return E_INVALIDARG;
	  }

	  if (MapType == D3D10_MAP_WRITE_DISCARD) {
		  // Allocate a new backing slice for the buffer and set
		  // it as the 'new' mapped slice. This assumes that the
		  // only way to invalidate a buffer is by mapping it.
		  auto physicalSlice = buffer->allocPhysicalSlice();
		  GetBufferInfo()->mappedSlice = physicalSlice;

		  m_device->EmitCs([
			  cBuffer = buffer,
				  cPhysicalSlice = physicalSlice
		  ] (DxvkContext* ctx) {
			  ctx->invalidateBuffer(cBuffer, cPhysicalSlice);
		  });
	  }
	  else if (MapType != D3D10_MAP_WRITE_NO_OVERWRITE) {
		  if (!m_device->WaitForResource(buffer->resource(), MapFlags))
			  return DXGI_ERROR_WAS_STILL_DRAWING;
	  }

	  // Use map pointer from previous map operation. This
	  // way we don't have to synchronize with the CS thread
	  // if the map mode is D3D11_MAP_WRITE_NO_OVERWRITE.
	  const DxvkPhysicalBufferSlice physicalSlice
		  = GetBufferInfo()->mappedSlice;

	  *ppData = physicalSlice.mapPtr(0);
	  return S_OK;
  }
    
  void STDMETHODCALLTYPE D3D10Buffer::Unmap(void)
  {
    Logger::warn("D3D10Buffer::Unmap: Stub"); 
  }
  
  Rc<DxvkBuffer> D3D10Buffer::CreateBuffer(
    const D3D10_BUFFER_DESC* pDesc) const {
    DxvkBufferCreateInfo  info;
    info.size   = pDesc->ByteWidth;
    info.usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
    info.access = VK_ACCESS_TRANSFER_READ_BIT
                | VK_ACCESS_TRANSFER_WRITE_BIT;
    
    if (pDesc->BindFlags & D3D10_BIND_VERTEX_BUFFER) {
      info.usage  |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      info.stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      info.access |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    
    if (pDesc->BindFlags & D3D10_BIND_INDEX_BUFFER) {
      info.usage  |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      info.stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      info.access |= VK_ACCESS_INDEX_READ_BIT;
    }
    
    if (pDesc->BindFlags & D3D10_BIND_CONSTANT_BUFFER) {
      info.usage  |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      info.stages |= m_device->GetEnabledShaderStages();
      info.access |= VK_ACCESS_UNIFORM_READ_BIT;
    }
    
    if (pDesc->BindFlags & D3D10_BIND_SHADER_RESOURCE) {
      info.usage  |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
      info.stages |= m_device->GetEnabledShaderStages();
      info.access |= VK_ACCESS_SHADER_READ_BIT;
    }
    
    if (pDesc->BindFlags & D3D10_BIND_STREAM_OUTPUT) {
      Logger::err("D3D10Device::CreateBuffer: D3D10_BIND_STREAM_OUTPUT not supported");
    }
    
    if (pDesc->CPUAccessFlags & D3D10_CPU_ACCESS_WRITE) {
      info.stages |= VK_PIPELINE_STAGE_HOST_BIT;
      info.access |= VK_ACCESS_HOST_WRITE_BIT;
    }
    
    if (pDesc->CPUAccessFlags & D3D10_CPU_ACCESS_READ) {
      info.stages |= VK_PIPELINE_STAGE_HOST_BIT;
      info.access |= VK_ACCESS_HOST_READ_BIT;
    }
    
    return m_device->GetDXVKDevice()->createBuffer(
      info, GetMemoryFlags(pDesc));
  }
  
  
  VkMemoryPropertyFlags D3D10Buffer::GetMemoryFlags(
    const D3D10_BUFFER_DESC* pDesc) const {
    // Default constant buffers may get updated frequently with calls
    // to D3D10DeviceContext::UpdateSubresource, so we'll map them to
    // host memory in order to allow direct access to the buffer
    if ((pDesc->Usage == D3D10_USAGE_DEFAULT)
     && (pDesc->BindFlags & D3D10_BIND_CONSTANT_BUFFER)) {
      return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
           | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    
    // Use default memory flags for the intended use
    return GetMemoryFlagsForUsage(pDesc->Usage);
  }
  
}
