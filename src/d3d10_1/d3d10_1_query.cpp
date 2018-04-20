#include "d3d10_1_device.h"
#include "d3d10_1_query.h"

namespace dxvk {
  
  D3D10Query::D3D10Query(
          D3D10Device*      device,
    const D3D10_QUERY_DESC& desc)
  : m_device(device), m_desc(desc) {
    switch (m_desc.Query) {
      case D3D10_QUERY_EVENT:
        m_event = new DxvkEvent();
        break;
        
      case D3D10_QUERY_OCCLUSION:
        m_query = new DxvkQuery(
          VK_QUERY_TYPE_OCCLUSION,
          VK_QUERY_CONTROL_PRECISE_BIT);
        break;
      
      case D3D10_QUERY_OCCLUSION_PREDICATE:
        m_query = new DxvkQuery(
          VK_QUERY_TYPE_OCCLUSION, 0);
        break;
        
      case D3D10_QUERY_TIMESTAMP:
        m_query = new DxvkQuery(
          VK_QUERY_TYPE_TIMESTAMP, 0);
        break;
      
      case D3D10_QUERY_TIMESTAMP_DISJOINT:
        break;
      
      case D3D10_QUERY_PIPELINE_STATISTICS:
        m_query = new DxvkQuery(
          VK_QUERY_TYPE_PIPELINE_STATISTICS, 0);
        break;
      
      default:
        throw DxvkError(str::format("D3D10: Unhandled query type: ", desc.Query));
    }
  }
  
  
  D3D10Query::~D3D10Query() {
    
  }
  
    
  HRESULT STDMETHODCALLTYPE D3D10Query::QueryInterface(REFIID  riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(ID3D10DeviceChild)
     || riid == __uuidof(ID3D10Asynchronous)
     || riid == __uuidof(ID3D10Query)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    if (riid == __uuidof(ID3D10Predicate)
     && m_desc.Query == D3D10_QUERY_OCCLUSION_PREDICATE) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    Logger::warn("D3D10Query: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  
  
  void STDMETHODCALLTYPE D3D10Query::GetDevice(ID3D10Device **ppDevice) {
    *ppDevice = ref(m_device);
  }
  
  
  UINT STDMETHODCALLTYPE D3D10Query::GetDataSize() {
    switch (m_desc.Query) {
      case D3D10_QUERY_EVENT:
        return sizeof(BOOL);
      
      case D3D10_QUERY_OCCLUSION:
        return sizeof(UINT64);
      
      case D3D10_QUERY_TIMESTAMP:
        return sizeof(UINT64);
      
      case D3D10_QUERY_TIMESTAMP_DISJOINT:
        return sizeof(D3D10_QUERY_DATA_TIMESTAMP_DISJOINT);
      
      case D3D10_QUERY_PIPELINE_STATISTICS:
        return sizeof(D3D10_QUERY_DATA_PIPELINE_STATISTICS);
      
      case D3D10_QUERY_OCCLUSION_PREDICATE:
        return sizeof(BOOL);
      
      case D3D10_QUERY_SO_STATISTICS:
        return sizeof(D3D10_QUERY_DATA_SO_STATISTICS);
      
      case D3D10_QUERY_SO_OVERFLOW_PREDICATE:
        return sizeof(BOOL);
    }
    
    Logger::err("D3D10Query: Failed to query data size");
    return 0;
  }
  
    
  void STDMETHODCALLTYPE D3D10Query::GetDesc(D3D10_QUERY_DESC *pDesc) {
    *pDesc = m_desc;
  }
  
  
  uint32_t D3D10Query::Reset() {
    if (m_query != nullptr)
      return m_query->reset();
    
    if (m_event != nullptr)
      return m_event->reset();
    
    return 0;
  }
  
  
  bool D3D10Query::HasBeginEnabled() const {
    return m_desc.Query == D3D10_QUERY_OCCLUSION
        || m_desc.Query == D3D10_QUERY_OCCLUSION_PREDICATE
        || m_desc.Query == D3D10_QUERY_PIPELINE_STATISTICS;
  }
  
  
  void D3D10Query::Begin(DxvkContext* ctx, uint32_t revision) {
    m_revision = revision;
    
    if (m_query != nullptr) {
      DxvkQueryRevision rev = { m_query, revision };
      ctx->beginQuery(rev);
    }
  }
  
  
  void D3D10Query::End(DxvkContext* ctx) {
    if (m_query != nullptr) {
      DxvkQueryRevision rev = { m_query, m_revision };
      ctx->endQuery(rev);
    }
  }
  
  
  void D3D10Query::Signal(DxvkContext* ctx, uint32_t revision) {
    switch (m_desc.Query) {
      case D3D10_QUERY_EVENT: {
        DxvkEventRevision rev = { m_event, revision };
        ctx->signalEvent(rev);
      } break;
      
      case D3D10_QUERY_TIMESTAMP: {
        DxvkQueryRevision rev = { m_query, revision };
        ctx->writeTimestamp(rev);
      } break;
      
      default:
        break;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Query::GetData(
          void*                             pData,
		  UINT								DataSize,
          UINT                              GetDataFlags) {
    if (m_desc.Query == D3D10_QUERY_EVENT) {
      const bool signaled = m_event->getStatus() == DxvkEventStatus::Signaled;
      
      if (pData != nullptr)
        *static_cast<BOOL*>(pData) = signaled;
      
      return signaled ? S_OK : S_FALSE;
    } else {
      DxvkQueryData queryData = {};
      
      if (m_query                     != nullptr
       && m_query->getData(queryData) != DxvkQueryStatus::Available)
        return S_FALSE;
      
      if (pData == nullptr)
        return S_OK;
      
      switch (m_desc.Query) {
        case D3D10_QUERY_OCCLUSION:
          *static_cast<UINT64*>(pData) = queryData.occlusion.samplesPassed;
          return S_OK;
        
        case D3D10_QUERY_OCCLUSION_PREDICATE:
          *static_cast<BOOL*>(pData) = queryData.occlusion.samplesPassed != 0;
          return S_OK;
        
        case D3D10_QUERY_TIMESTAMP:
          *static_cast<UINT64*>(pData) = queryData.timestamp.time;
          return S_OK;
        
        case D3D10_QUERY_TIMESTAMP_DISJOINT: {
          // FIXME return correct frequency
          auto data = static_cast<D3D10_QUERY_DATA_TIMESTAMP_DISJOINT*>(pData);
          data->Frequency = 1000;
          data->Disjoint = FALSE;
        } return S_OK;
        
        case D3D10_QUERY_PIPELINE_STATISTICS: {
          auto data = static_cast<D3D10_QUERY_DATA_PIPELINE_STATISTICS*>(pData);
          data->IAVertices    = queryData.statistic.iaVertices;
          data->IAPrimitives  = queryData.statistic.iaPrimitives;
          data->VSInvocations = queryData.statistic.vsInvocations;
          data->GSInvocations = queryData.statistic.gsInvocations;
          data->GSPrimitives  = queryData.statistic.gsPrimitives;
          data->CInvocations  = queryData.statistic.clipInvocations;
          data->CPrimitives   = queryData.statistic.clipPrimitives;
          data->PSInvocations = queryData.statistic.fsInvocations;
        } return S_OK;
          
        default:
          Logger::err(str::format("D3D10: Unhandled query type in GetData: ", m_desc.Query));
          return E_INVALIDARG;
      }
    }
  }
  
  void STDMETHODCALLTYPE D3D10Query::Begin(void)
  {
	  Logger::warn("D3D10Query::Begin: Stub");
  }

  void STDMETHODCALLTYPE D3D10Query::End(void)
  {
	  Logger::warn("D3D10Query::End: Stub");
  }
}
