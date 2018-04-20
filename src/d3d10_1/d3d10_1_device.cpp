#include <algorithm>
#include <cstring>

#include "d3d10_1_buffer.h"
#include "d3d10_1_device.h"
#include "d3d10_1_input_layout.h"
#include "d3d10_1_present.h"
#include "d3d10_1_query.h"
#include "d3d10_1_sampler.h"
#include "d3d10_1_shader.h"
#include "d3d10_1_texture.h"

#include "d3d10_1_view_dsv.h"
#include "d3d10_1_view_rtv.h"
#include "d3d10_1_view_srv.h"
	
namespace dxvk {
  
  D3D10DeviceContainer::D3D10DeviceContainer() {
    
  }
  
  
  D3D10DeviceContainer::~D3D10DeviceContainer() {
    delete m_d3d10Presenter;
    delete m_d3d10Device;
    delete m_dxgiDevice;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10DeviceContainer::QueryInterface(REFIID riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(IDXGIObject)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    if (riid == __uuidof(IDXGIDevice)
     || riid == __uuidof(IDXGIDevice1)
     || riid == __uuidof(IDXGIDevice2)
     || riid == __uuidof(IDXGIVkDevice)) {
      *ppvObject = ref(m_dxgiDevice);
      return S_OK;
    }
    
    if (riid == __uuidof(ID3D10Device)
     || riid == __uuidof(ID3D10Device1)) {
      *ppvObject = ref(m_d3d10Device);
      return S_OK;
    }
    
    if (riid == __uuidof(IDXGIVkPresenter)) {
      *ppvObject = ref(m_d3d10Presenter);
      return S_OK;
    }
    
    if (riid == __uuidof(ID3D10Debug))
      return E_NOINTERFACE;      
    
    // Undocumented interfaces that are queried by some games
    if (riid == GUID{0xd56e2a4c,0x5127,0x8437,{0x65,0x8a,0x98,0xc5,0xbb,0x78,0x94,0x98}})
      return E_NOINTERFACE;
    
    Logger::warn("D3D10DeviceContainer::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  

  HRESULT STDMETHODCALLTYPE D3D10DeviceContainer::GetParent(
          REFIID                  riid,
          void**                  ppParent) {
    return m_dxgiDevice->GetParent(riid, ppParent);
  }
  
  
  D3D10Device::D3D10Device(
          IDXGIObject*        pContainer,
          IDXGIVkDevice*      pDxgiDevice,
          D3D10_FEATURE_LEVEL1   FeatureLevel,
          UINT                FeatureFlags)
  : m_container     (pContainer),
    m_featureLevel  (FeatureLevel),
    m_featureFlags  (FeatureFlags),
    m_dxvkDevice    (pDxgiDevice->GetDXVKDevice()),
    m_dxvkAdapter   (m_dxvkDevice->adapter()),
    m_d3d10Options  (D3D10GetAppOptions(env::getExeName())),
    m_dxbcOptions   (m_dxvkDevice) {
    Com<IDXGIAdapter> adapter;
    
    if (FAILED(pDxgiDevice->GetAdapter(&adapter))
     || FAILED(adapter->QueryInterface(__uuidof(IDXGIVkAdapter),
          reinterpret_cast<void**>(&m_dxgiAdapter))))
      throw DxvkError("D3D10Device: Failed to query adapter");
    
	m_csThread = new DxvkCsThread(m_dxvkDevice->createContext());

	EmitCs([cDevice = m_dxvkDevice](DxvkContext* ctx) {
		ctx->beginRecording(cDevice->createCommandList());
	});

    m_resourceInitContext = m_dxvkDevice->createContext();
    m_resourceInitContext->beginRecording(
      m_dxvkDevice->createCommandList());

    Com<ID3D10BlendState>         defaultBlendState;
    Com<ID3D10DepthStencilState>  defaultDepthStencilState;
    Com<ID3D10RasterizerState>    defaultRasterizerState;
    
    if (FAILED(CreateBlendState       (nullptr, &defaultBlendState))
     || FAILED(CreateDepthStencilState(nullptr, &defaultDepthStencilState))
     || FAILED(CreateRasterizerState  (nullptr, &defaultRasterizerState)))
      throw DxvkError("D3D10Device: Failed to create default state objects");
    
    // Apply default state to the context. This is required
    // in order to initialize the DXVK contex properly.
    m_defaultBlendState        = static_cast<D3D10BlendState*>       (defaultBlendState.ptr());
    m_defaultDepthStencilState = static_cast<D3D10DepthStencilState*>(defaultDepthStencilState.ptr());
    m_defaultRasterizerState   = static_cast<D3D10RasterizerState*>  (defaultRasterizerState.ptr());
  }
  
  
  D3D10Device::~D3D10Device() {
  }
  
  
  ULONG STDMETHODCALLTYPE D3D10Device::AddRef() {
    return m_container->AddRef();
  }
  
  
  ULONG STDMETHODCALLTYPE D3D10Device::Release() {
    return m_container->Release();
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::QueryInterface(REFIID riid, void** ppvObject) {
    return m_container->QueryInterface(riid, ppvObject);
  }
    
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateBuffer(
    const D3D10_BUFFER_DESC*      pDesc,
    const D3D10_SUBRESOURCE_DATA* pInitialData,
          ID3D10Buffer**          ppBuffer) {
    InitReturnPtr(ppBuffer);
    
    if (ppBuffer == nullptr)
      return S_FALSE;
    
    try {
      const Com<D3D10Buffer> buffer
        = new D3D10Buffer(this, pDesc);
      
      this->InitBuffer(buffer.ptr(), pInitialData);
      *ppBuffer = buffer.ref();
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateTexture1D(
    const D3D10_TEXTURE1D_DESC*   pDesc,
    const D3D10_SUBRESOURCE_DATA* pInitialData,
          ID3D10Texture1D**       ppTexture1D) {
    InitReturnPtr(ppTexture1D);
    
    D3D10_COMMON_TEXTURE_DESC desc;
    desc.Width          = pDesc->Width;
    desc.Height         = 1;
    desc.Depth          = 1;
    desc.MipLevels      = pDesc->MipLevels;
    desc.ArraySize      = pDesc->ArraySize;
    desc.Format         = pDesc->Format;
    desc.SampleDesc     = DXGI_SAMPLE_DESC { 1, 0 };
    desc.Usage          = pDesc->Usage;
    desc.BindFlags      = pDesc->BindFlags;
    desc.CPUAccessFlags = pDesc->CPUAccessFlags;
    desc.MiscFlags      = pDesc->MiscFlags;
    
    if (FAILED(D3D10CommonTexture::NormalizeTextureProperties(&desc)))
      return E_INVALIDARG;
    
    if (ppTexture1D == nullptr)
      return S_FALSE;
    
    try {
      const Com<D3D10Texture1D> texture = new D3D10Texture1D(this, &desc);
      this->InitTexture(texture->GetCommonTexture()->GetImage(), pInitialData);
      *ppTexture1D = texture.ref();
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateTexture2D(
    const D3D10_TEXTURE2D_DESC*   pDesc,
    const D3D10_SUBRESOURCE_DATA* pInitialData,
          ID3D10Texture2D**       ppTexture2D) {
    InitReturnPtr(ppTexture2D);
    
    D3D10_COMMON_TEXTURE_DESC desc;
    desc.Width          = pDesc->Width;
    desc.Height         = pDesc->Height;
    desc.Depth          = 1;
    desc.MipLevels      = pDesc->MipLevels;
    desc.ArraySize      = pDesc->ArraySize;
    desc.Format         = pDesc->Format;
    desc.SampleDesc     = pDesc->SampleDesc;
    desc.Usage          = pDesc->Usage;
    desc.BindFlags      = pDesc->BindFlags;
    desc.CPUAccessFlags = pDesc->CPUAccessFlags;
    desc.MiscFlags      = pDesc->MiscFlags;
    
    if (FAILED(D3D10CommonTexture::NormalizeTextureProperties(&desc)))
      return E_INVALIDARG;
    
    if (ppTexture2D == nullptr)
      return S_FALSE;
    
    try {
      const Com<D3D10Texture2D> texture = new D3D10Texture2D(this, &desc);
      this->InitTexture(texture->GetCommonTexture()->GetImage(), pInitialData);
      *ppTexture2D = texture.ref();
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateTexture3D(
    const D3D10_TEXTURE3D_DESC*   pDesc,
    const D3D10_SUBRESOURCE_DATA* pInitialData,
          ID3D10Texture3D**       ppTexture3D) {
    InitReturnPtr(ppTexture3D);
    
    D3D10_COMMON_TEXTURE_DESC desc;
    desc.Width          = pDesc->Width;
    desc.Height         = pDesc->Height;
    desc.Depth          = pDesc->Depth;
    desc.MipLevels      = pDesc->MipLevels;
    desc.ArraySize      = 1;
    desc.Format         = pDesc->Format;
    desc.SampleDesc     = DXGI_SAMPLE_DESC { 1, 0 };
    desc.Usage          = pDesc->Usage;
    desc.BindFlags      = pDesc->BindFlags;
    desc.CPUAccessFlags = pDesc->CPUAccessFlags;
    desc.MiscFlags      = pDesc->MiscFlags;
    
    if (FAILED(D3D10CommonTexture::NormalizeTextureProperties(&desc)))
      return E_INVALIDARG;
    
    if (ppTexture3D == nullptr)
      return S_FALSE;
      
    try {
      const Com<D3D10Texture3D> texture = new D3D10Texture3D(this, &desc);
      this->InitTexture(texture->GetCommonTexture()->GetImage(), pInitialData);
      *ppTexture3D = texture.ref();
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateShaderResourceView1(
          ID3D10Resource*                   pResource,
    const D3D10_SHADER_RESOURCE_VIEW_DESC1*  pDesc,
          ID3D10ShaderResourceView1**        ppSRView) {
    InitReturnPtr(ppSRView);
    
    D3D10_RESOURCE_DIMENSION resourceDim = D3D10_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&resourceDim);
    
    // The description is optional. If omitted, we'll create
    // a view that covers all subresources of the image.
    D3D10_SHADER_RESOURCE_VIEW_DESC1 desc;
    
    if (pDesc == nullptr) {
      if (FAILED(D3D10ShaderResourceView::GetDescFromResource(pResource, &desc)))
        return E_INVALIDARG;
    } else {
      desc = *pDesc;
      
      if (FAILED(D3D10ShaderResourceView::NormalizeDesc(pResource, &desc)))
        return E_INVALIDARG;
    }
    
    if (resourceDim == D3D10_RESOURCE_DIMENSION_BUFFER) {
      auto resource = static_cast<D3D10Buffer*>(pResource);
      
      D3D10_BUFFER_DESC resourceDesc;
      resource->GetDesc(&resourceDesc);
      
      if ((resourceDesc.BindFlags & D3D10_BIND_SHADER_RESOURCE) == 0) {
        Logger::warn("D3D10: Trying to create SRV for buffer without D3D10_BIND_SHADER_RESOURCE");
        return E_INVALIDARG;
      }
      
      DxvkBufferViewCreateInfo viewInfo;
      
      D3D10_BUFFER_SRV bufInfo;
      
      if (desc.ViewDimension == D3D10_SRV_DIMENSION_BUFFER) {
        bufInfo.FirstElement = desc.Buffer.FirstElement;
        bufInfo.NumElements  = desc.Buffer.NumElements;
      } else {
        Logger::err("D3D10Device: Invalid buffer view dimension");
        return E_INVALIDARG;
      }
      
      /*if (bufInfo.Flags & D3D10_BUFFEREX_SRV_FLAG_RAW) {
        // Raw buffer view. We'll represent this as a
        // uniform texel buffer with UINT32 elements.
        viewInfo.format = VK_FORMAT_R32_UINT;
        viewInfo.rangeOffset = sizeof(uint32_t) * bufInfo.FirstElement;
        viewInfo.rangeLength = sizeof(uint32_t) * bufInfo.NumElements;
      } else*/if (desc.Format == DXGI_FORMAT_UNKNOWN) {
        // Structured buffer view
        viewInfo.format = VK_FORMAT_R32_UINT;
        viewInfo.rangeOffset = /*resourceDesc.StructureByteStride*/ sizeof(uint32_t) * bufInfo.FirstElement;
        viewInfo.rangeLength = /*resourceDesc.StructureByteStride*/ sizeof(uint32_t) * bufInfo.NumElements;
      } else {
        // Typed buffer view - must use an uncompressed color format
        viewInfo.format = m_dxgiAdapter->LookupFormat(
          desc.Format, DXGI_VK_FORMAT_MODE_COLOR).Format;
        
        const DxvkFormatInfo* formatInfo = imageFormatInfo(viewInfo.format);
        viewInfo.rangeOffset = formatInfo->elementSize * bufInfo.FirstElement;
        viewInfo.rangeLength = formatInfo->elementSize * bufInfo.NumElements;
        
        if (formatInfo->flags.test(DxvkFormatFlag::BlockCompressed)) {
          Logger::err("D3D10Device: Compressed formats for buffer views not supported");
          return E_INVALIDARG;
        }
      }
      
      if (ppSRView == nullptr)
        return S_FALSE;
      
      try {
        *ppSRView = ref(new D3D10ShaderResourceView(
          this, pResource, desc,
          m_dxvkDevice->createBufferView(
            resource->GetBufferSlice().buffer(), viewInfo)));
        return S_OK;
      } catch (const DxvkError& e) {
        Logger::err(e.message());
        return E_FAIL;
      }
    } else {
      const D3D10CommonTexture* textureInfo = GetCommonTexture(pResource);
      
      if ((textureInfo->Desc()->BindFlags & D3D10_BIND_SHADER_RESOURCE) == 0) {
        Logger::warn("D3D10: Trying to create SRV for texture without D3D10_BIND_SHADER_RESOURCE");
        return E_INVALIDARG;
      }
      
      // Fill in the view info. The view type depends solely
      // on the view dimension field in the view description,
      // not on the resource type.
      const DXGI_VK_FORMAT_INFO formatInfo = m_dxgiAdapter
        ->LookupFormat(desc.Format, textureInfo->GetFormatMode());
      
      DxvkImageViewCreateInfo viewInfo;
      viewInfo.format  = formatInfo.Format;
      viewInfo.aspect  = formatInfo.Aspect;
      viewInfo.swizzle = formatInfo.Swizzle;
      
      switch (desc.ViewDimension) {
        case D3D10_SRV_DIMENSION_TEXTURE1D:
          viewInfo.type      = VK_IMAGE_VIEW_TYPE_1D;
          viewInfo.minLevel  = desc.Texture1D.MostDetailedMip;
          viewInfo.numLevels = desc.Texture1D.MipLevels;
          viewInfo.minLayer  = 0;
          viewInfo.numLayers = 1;
          break;
          
        case D3D10_SRV_DIMENSION_TEXTURE1DARRAY:
          viewInfo.type      = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
          viewInfo.minLevel  = desc.Texture1DArray.MostDetailedMip;
          viewInfo.numLevels = desc.Texture1DArray.MipLevels;
          viewInfo.minLayer  = desc.Texture1DArray.FirstArraySlice;
          viewInfo.numLayers = desc.Texture1DArray.ArraySize;
          break;
          
        case D3D10_SRV_DIMENSION_TEXTURE2D:
          viewInfo.type      = VK_IMAGE_VIEW_TYPE_2D;
          viewInfo.minLevel  = desc.Texture2D.MostDetailedMip;
          viewInfo.numLevels = desc.Texture2D.MipLevels;
          viewInfo.minLayer  = 0;
          viewInfo.numLayers = 1;
          break;
          
        case D3D10_SRV_DIMENSION_TEXTURE2DARRAY:
          viewInfo.type      = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
          viewInfo.minLevel  = desc.Texture2DArray.MostDetailedMip;
          viewInfo.numLevels = desc.Texture2DArray.MipLevels;
          viewInfo.minLayer  = desc.Texture2DArray.FirstArraySlice;
          viewInfo.numLayers = desc.Texture2DArray.ArraySize;
          break;
          
        case D3D10_SRV_DIMENSION_TEXTURE2DMS:
          viewInfo.type      = VK_IMAGE_VIEW_TYPE_2D;
          viewInfo.minLevel  = 0;
          viewInfo.numLevels = 1;
          viewInfo.minLayer  = 0;
          viewInfo.numLayers = 1;
          break;
          
        case D3D10_SRV_DIMENSION_TEXTURE2DMSARRAY:
          viewInfo.type      = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
          viewInfo.minLevel  = 0;
          viewInfo.numLevels = 1;
          viewInfo.minLayer  = desc.Texture2DMSArray.FirstArraySlice;
          viewInfo.numLayers = desc.Texture2DMSArray.ArraySize;
          break;
          
        case D3D10_SRV_DIMENSION_TEXTURE3D:
          viewInfo.type      = VK_IMAGE_VIEW_TYPE_3D;
          viewInfo.minLevel  = desc.Texture3D.MostDetailedMip;
          viewInfo.numLevels = desc.Texture3D.MipLevels;
          viewInfo.minLayer  = 0;
          viewInfo.numLayers = 1;
          break;
          
        case D3D10_SRV_DIMENSION_TEXTURECUBE:
          viewInfo.type      = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
          viewInfo.minLevel  = desc.TextureCube.MostDetailedMip;
          viewInfo.numLevels = desc.TextureCube.MipLevels;
          viewInfo.minLayer  = 0;
          viewInfo.numLayers = 6;
          break;
          
        case D3D10_1_SRV_DIMENSION_TEXTURECUBEARRAY:
          viewInfo.type      = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
          viewInfo.minLevel  = desc.TextureCubeArray.MostDetailedMip;
          viewInfo.numLevels = desc.TextureCubeArray.MipLevels;
          viewInfo.minLayer  = desc.TextureCubeArray.First2DArrayFace;
          viewInfo.numLayers = desc.TextureCubeArray.NumCubes * 6;
          break;
          
        default:
          Logger::err(str::format(
            "D3D10: View dimension not supported for SRV: ",
            desc.ViewDimension));
          return E_INVALIDARG;
      }
      
      if (ppSRView == nullptr)
        return S_FALSE;
      
      try {
        *ppSRView = ref(new D3D10ShaderResourceView(
          this, pResource, desc,
          m_dxvkDevice->createImageView(
            textureInfo->GetImage(),
            viewInfo)));
        return S_OK;
      } catch (const DxvkError& e) {
        Logger::err(e.message());
        return E_FAIL;
      }
    }
  }

  void D3D10Device::FlushInitContext() {
	  LockResourceInitContext();
	  if (m_resourceInitCommands != 0)
		  SubmitResourceInitCommands();
	  UnlockResourceInitContext(0);
  }

  HRESULT STDMETHODCALLTYPE D3D10Device::CreateShaderResourceView(
          ID3D10Resource*                   pResource,
    const D3D10_SHADER_RESOURCE_VIEW_DESC*  pDesc,
          ID3D10ShaderResourceView**        ppSRView) {
	  D3D10_SHADER_RESOURCE_VIEW_DESC1 newDesc = {};
	  newDesc.Buffer = pDesc->Buffer;
	  newDesc.Format = pDesc->Format;
	  newDesc.Texture1D = pDesc->Texture1D;
	  newDesc.Texture1DArray = pDesc->Texture1DArray;
	  newDesc.Texture2D = pDesc->Texture2D;
	  newDesc.Texture2DMS = pDesc->Texture2DMS;
	  newDesc.Texture2DMSArray = pDesc->Texture2DMSArray;
	  newDesc.Texture2DArray = pDesc->Texture2DArray;
	  newDesc.Texture3D = pDesc->Texture3D;
	  newDesc.TextureCube = pDesc->TextureCube;
	  newDesc.ViewDimension = pDesc->ViewDimension;

	  return CreateShaderResourceView1(pResource, &newDesc, (ID3D10ShaderResourceView1**)ppSRView);
	}
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateRenderTargetView(
          ID3D10Resource*                   pResource,
    const D3D10_RENDER_TARGET_VIEW_DESC*    pDesc,
          ID3D10RenderTargetView**          ppRTView) {
    InitReturnPtr(ppRTView);
    
    // DXVK only supports render target views for image resources
    D3D10_RESOURCE_DIMENSION resourceDim = D3D10_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&resourceDim);
    
    if (resourceDim == D3D10_RESOURCE_DIMENSION_BUFFER) {
      Logger::warn("D3D10: Cannot create render target view for a buffer");
      return S_OK; // It is required to run Battlefield 3 and Battlefield 4.
    }
    
    // The view description is optional. If not defined, it
    // will use the resource's format and all array layers.
    D3D10_RENDER_TARGET_VIEW_DESC desc;
    
    if (pDesc == nullptr) {
      if (FAILED(D3D10RenderTargetView::GetDescFromResource(pResource, &desc)))
        return E_INVALIDARG;
    } else {
      desc = *pDesc;
      
      if (FAILED(D3D10RenderTargetView::NormalizeDesc(pResource, &desc)))
        return E_INVALIDARG;
    }
    
    // Retrieve the image that we are going to create the view for
    const D3D10CommonTexture* textureInfo = GetCommonTexture(pResource);
    
    if ((textureInfo->Desc()->BindFlags & D3D10_BIND_RENDER_TARGET) == 0) {
      Logger::warn("D3D10: Trying to create RTV for texture without D3D10_BIND_RENDER_TARGET");
      return E_INVALIDARG;
    }
    
    // Fill in Vulkan image view info
    DxvkImageViewCreateInfo viewInfo;
    viewInfo.format = m_dxgiAdapter->LookupFormat(desc.Format, DXGI_VK_FORMAT_MODE_COLOR).Format;
    viewInfo.aspect = imageFormatInfo(viewInfo.format)->aspectMask;
    
    switch (desc.ViewDimension) {
      case D3D10_RTV_DIMENSION_TEXTURE1D:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_1D;
        viewInfo.minLevel   = desc.Texture1D.MipSlice;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = 0;
        viewInfo.numLayers  = 1;
        break;
        
      case D3D10_RTV_DIMENSION_TEXTURE1DARRAY:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        viewInfo.minLevel   = desc.Texture1DArray.MipSlice;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = desc.Texture1DArray.FirstArraySlice;
        viewInfo.numLayers  = desc.Texture1DArray.ArraySize;
        break;
        
      case D3D10_RTV_DIMENSION_TEXTURE2D:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.minLevel   = desc.Texture2D.MipSlice;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = 0;
        viewInfo.numLayers  = 1;
        break;
        
      case D3D10_RTV_DIMENSION_TEXTURE2DARRAY:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.minLevel   = desc.Texture2DArray.MipSlice;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = desc.Texture2DArray.FirstArraySlice;
        viewInfo.numLayers  = desc.Texture2DArray.ArraySize;
        break;
        
      case D3D10_RTV_DIMENSION_TEXTURE2DMS:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.minLevel   = 0;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = 0;
        viewInfo.numLayers  = 1;
        break;
      
      case D3D10_RTV_DIMENSION_TEXTURE2DMSARRAY:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.minLevel   = 0;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = desc.Texture2DMSArray.FirstArraySlice;
        viewInfo.numLayers  = desc.Texture2DMSArray.ArraySize;
        break;
      
      case D3D10_RTV_DIMENSION_TEXTURE3D:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.minLevel   = desc.Texture3D.MipSlice;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = desc.Texture3D.FirstWSlice;
        viewInfo.numLayers  = desc.Texture3D.WSize;
        break;
      
      default:
        Logger::err(str::format(
          "D3D10: pDesc->ViewDimension not supported for render target views: ",
          desc.ViewDimension));
        return E_INVALIDARG;
    }
    
    // Create the actual image view if requested
    if (ppRTView == nullptr)
      return S_FALSE;
    
    try {
      *ppRTView = ref(new D3D10RenderTargetView(
        this, pResource, desc,
        m_dxvkDevice->createImageView(
          textureInfo->GetImage(),
          viewInfo)));
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateDepthStencilView(
          ID3D10Resource*                   pResource,
    const D3D10_DEPTH_STENCIL_VIEW_DESC*    pDesc,
          ID3D10DepthStencilView**          ppDepthStencilView) {
    InitReturnPtr(ppDepthStencilView);
    
    D3D10_RESOURCE_DIMENSION resourceDim = D3D10_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&resourceDim);
    
    // The view description is optional. If not defined, it
    // will use the resource's format and all array layers.
    D3D10_DEPTH_STENCIL_VIEW_DESC desc;
    
    if (pDesc == nullptr) {
      if (FAILED(D3D10DepthStencilView::GetDescFromResource(pResource, &desc)))
        return E_INVALIDARG;
    } else {
      desc = *pDesc;
      
      if (FAILED(D3D10DepthStencilView::NormalizeDesc(pResource, &desc)))
        return E_INVALIDARG;
    }
    
    // Retrieve the image that we are going to create the view for
    const D3D10CommonTexture* textureInfo = GetCommonTexture(pResource);
    
    if ((textureInfo->Desc()->BindFlags & D3D10_BIND_DEPTH_STENCIL) == 0) {
      Logger::warn("D3D10: Trying to create DSV for texture without D3D10_BIND_DEPTH_STENCIL");
      return E_INVALIDARG;
    }
    
    // Fill in Vulkan image view info
    DxvkImageViewCreateInfo viewInfo;
    viewInfo.format = m_dxgiAdapter->LookupFormat(desc.Format, DXGI_VK_FORMAT_MODE_DEPTH).Format;
    viewInfo.aspect = imageFormatInfo(viewInfo.format)->aspectMask;
    
    switch (desc.ViewDimension) {
      case D3D10_DSV_DIMENSION_TEXTURE1D:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_1D;
        viewInfo.minLevel   = desc.Texture1D.MipSlice;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = 0;
        viewInfo.numLayers  = 1;
        break;
        
      case D3D10_DSV_DIMENSION_TEXTURE1DARRAY:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        viewInfo.minLevel   = desc.Texture1DArray.MipSlice;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = desc.Texture1DArray.FirstArraySlice;
        viewInfo.numLayers  = desc.Texture1DArray.ArraySize;
        break;
        
      case D3D10_DSV_DIMENSION_TEXTURE2D:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.minLevel   = desc.Texture2D.MipSlice;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = 0;
        viewInfo.numLayers  = 1;
        break;
        
      case D3D10_DSV_DIMENSION_TEXTURE2DARRAY:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.minLevel   = desc.Texture2DArray.MipSlice;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = desc.Texture2DArray.FirstArraySlice;
        viewInfo.numLayers  = desc.Texture2DArray.ArraySize;
        break;
        
      case D3D10_DSV_DIMENSION_TEXTURE2DMS:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.minLevel   = 0;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = 0;
        viewInfo.numLayers  = 1;
        break;
      
      case D3D10_DSV_DIMENSION_TEXTURE2DMSARRAY:
        viewInfo.type       = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.minLevel   = 0;
        viewInfo.numLevels  = 1;
        viewInfo.minLayer   = desc.Texture2DMSArray.FirstArraySlice;
        viewInfo.numLayers  = desc.Texture2DMSArray.ArraySize;
        break;
      
      default:
        Logger::err(str::format(
          "D3D10: pDesc->ViewDimension not supported for depth-stencil views: ",
          desc.ViewDimension));
        return E_INVALIDARG;
    }
    
    // Create the actual image view if requested
    if (ppDepthStencilView == nullptr)
      return S_FALSE;
    
    try {
      *ppDepthStencilView = ref(new D3D10DepthStencilView(
        this, pResource, desc,
        m_dxvkDevice->createImageView(
          textureInfo->GetImage(),
          viewInfo)));
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateInputLayout(
    const D3D10_INPUT_ELEMENT_DESC*   pInputElementDescs,
          UINT                        NumElements,
    const void*                       pShaderBytecodeWithInputSignature,
          SIZE_T                      BytecodeLength,
          ID3D10InputLayout**         ppInputLayout) {
    InitReturnPtr(ppInputLayout);
    
    try {
      DxbcReader dxbcReader(reinterpret_cast<const char*>(
        pShaderBytecodeWithInputSignature), BytecodeLength);
      DxbcModule dxbcModule(dxbcReader);
      
      const Rc<DxbcIsgn> inputSignature = dxbcModule.isgn();
      
      std::vector<DxvkVertexAttribute> attributes;
      std::vector<DxvkVertexBinding>   bindings;
      
      for (uint32_t i = 0; i < NumElements; i++) {
        const DxbcSgnEntry* entry = inputSignature->find(
          pInputElementDescs[i].SemanticName,
          pInputElementDescs[i].SemanticIndex);
        
        if (entry == nullptr) {
          Logger::debug(str::format(
            "D3D10Device: No such vertex shader semantic: ",
            pInputElementDescs[i].SemanticName,
            pInputElementDescs[i].SemanticIndex));
          continue;
        }
        
        // Create vertex input attribute description
        DxvkVertexAttribute attrib;
        attrib.location = entry->registerId;
        attrib.binding  = pInputElementDescs[i].InputSlot;
        attrib.format   = m_dxgiAdapter->LookupFormat(
          pInputElementDescs[i].Format, DXGI_VK_FORMAT_MODE_COLOR).Format;
        attrib.offset   = pInputElementDescs[i].AlignedByteOffset;
        
        // The application may choose to let the implementation
        // generate the exact vertex layout. In that case we'll
        // pack attributes on the same binding in the order they
        // are declared, aligning each attribute to four bytes.
        if (attrib.offset == D3D10_APPEND_ALIGNED_ELEMENT) {
          attrib.offset = 0;
          
          for (uint32_t j = 1; j <= i; j++) {
            const DxvkVertexAttribute& prev = attributes.at(i - j);
            
            if (prev.binding == attrib.binding) {
              const DxvkFormatInfo* formatInfo = imageFormatInfo(prev.format);
              attrib.offset = align(prev.offset + formatInfo->elementSize, 4);
              break;
            }
          }
        }
        
        attributes.push_back(attrib);
        
        // Create vertex input binding description. The
        // stride is dynamic state in D3D10 and will be
        // set by D3D10DeviceContext::IASetVertexBuffers.
        DxvkVertexBinding binding;
        binding.binding   = pInputElementDescs[i].InputSlot;
        binding.fetchRate = pInputElementDescs[i].InstanceDataStepRate;
        binding.inputRate = pInputElementDescs[i].InputSlotClass == D3D10_INPUT_PER_INSTANCE_DATA
          ? VK_VERTEX_INPUT_RATE_INSTANCE
          : VK_VERTEX_INPUT_RATE_VERTEX;
        
        // Check if the binding was already defined. If so, the
        // parameters must be identical (namely, the input rate).
        bool bindingDefined = false;
        
        for (const auto& existingBinding : bindings) {
          if (binding.binding == existingBinding.binding) {
            bindingDefined = true;
            
            if (binding.inputRate != existingBinding.inputRate) {
              Logger::err(str::format(
                "D3D10Device: Conflicting input rate for binding ",
                binding.binding));
              return E_INVALIDARG;
            }
          }
        }
        
        if (!bindingDefined)
          bindings.push_back(binding);
      }
      
      // Check if there are any semantics defined in the
      // shader that are not included in the current input
      // layout.
      for (auto i = inputSignature->begin(); i != inputSignature->end(); i++) {
        bool found = i->systemValue != DxbcSystemValue::None;
        
        for (uint32_t j = 0; j < attributes.size() && !found; j++)
          found = attributes.at(j).location == i->registerId;
        
        if (!found) {
          Logger::warn(str::format(
            "D3D10Device: Vertex input '",
            i->semanticName, i->semanticIndex,
            "' not defined by input layout"));
        }
      }
      
      std::sort(bindings.begin(), bindings.end(),
        [] (const DxvkVertexBinding& a, const DxvkVertexBinding& b) {
          return a.binding < b.binding;
        });
      
      // Create the actual input layout object
      // if the application requests it.
      if (ppInputLayout != nullptr) {
        *ppInputLayout = ref(
          new D3D10InputLayout(this,
            attributes.size(),
            attributes.data(),
            bindings.size(),
            bindings.data()));
      }
      
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateVertexShader(
    const void*                       pShaderBytecode,
          SIZE_T                      BytecodeLength,
          ID3D10VertexShader**        ppVertexShader) {
    InitReturnPtr(ppVertexShader);
    D3D10ShaderModule module;
    
    if (FAILED(this->CreateShaderModule(&module,
        pShaderBytecode, BytecodeLength,
        DxbcProgramType::VertexShader)))
      return E_INVALIDARG;
    
    if (ppVertexShader == nullptr)
      return S_FALSE;
    
    *ppVertexShader = ref(new D3D10VertexShader(
      this, std::move(module)));
    return S_OK;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateGeometryShader(
    const void*                       pShaderBytecode,
          SIZE_T                      BytecodeLength,
          ID3D10GeometryShader**      ppGeometryShader) {
    InitReturnPtr(ppGeometryShader);
    D3D10ShaderModule module;
    
    if (FAILED(this->CreateShaderModule(&module,
        pShaderBytecode, BytecodeLength,
        DxbcProgramType::GeometryShader)))
      return E_INVALIDARG;
    
    if (ppGeometryShader == nullptr)
      return S_FALSE;
    
    *ppGeometryShader = ref(new D3D10GeometryShader(
      this, std::move(module)));
    return S_OK;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateGeometryShaderWithStreamOutput(
    const void*                       pShaderBytecode,
          SIZE_T                      BytecodeLength,
    const D3D10_SO_DECLARATION_ENTRY* pSODeclaration,
          UINT                        NumEntries,
		  UINT                       OutputStreamStride,
          ID3D10GeometryShader**      ppGeometryShader) {
    InitReturnPtr(ppGeometryShader);
    Logger::err("D3D10Device::CreateGeometryShaderWithStreamOutput: Not implemented");
    return E_NOTIMPL;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreatePixelShader(
    const void*                       pShaderBytecode,
          SIZE_T                      BytecodeLength,
          ID3D10PixelShader**         ppPixelShader) {
    InitReturnPtr(ppPixelShader);
    D3D10ShaderModule module;
    
    if (FAILED(this->CreateShaderModule(&module,
        pShaderBytecode, BytecodeLength,
        DxbcProgramType::PixelShader)))
      return E_INVALIDARG;
    
    if (ppPixelShader == nullptr)
      return S_FALSE;
    
    *ppPixelShader = ref(new D3D10PixelShader(
      this, std::move(module)));
    return S_OK;
  }
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateBlendState(
    const D3D10_BLEND_DESC*           pBlendStateDesc,
          ID3D10BlendState**          ppBlendState) {
    InitReturnPtr(ppBlendState);
    
    D3D10_BLEND_DESC1 desc = pBlendStateDesc != nullptr
      ? D3D10BlendState::PromoteDesc(pBlendStateDesc)
      : D3D10BlendState::DefaultDesc();
    
    if (FAILED(D3D10BlendState::NormalizeDesc(&desc)))
      return E_INVALIDARG;
    
    if (ppBlendState != nullptr) {
      *ppBlendState = m_bsStateObjects.Create(this, desc);
      return S_OK;
    } return S_FALSE;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateBlendState1(
    const D3D10_BLEND_DESC1*          pBlendStateDesc, 
          ID3D10BlendState1**         ppBlendState) {
    InitReturnPtr(ppBlendState);
    
    D3D10_BLEND_DESC1 desc = pBlendStateDesc != nullptr
      ? *pBlendStateDesc
      : D3D10BlendState::DefaultDesc();
    
    if (FAILED(D3D10BlendState::NormalizeDesc(&desc)))
      return E_INVALIDARG;
    
    if (ppBlendState != nullptr) {
      *ppBlendState = m_bsStateObjects.Create(this, desc);
      return S_OK;
    } return S_FALSE;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateDepthStencilState(
    const D3D10_DEPTH_STENCIL_DESC*   pDepthStencilDesc,
          ID3D10DepthStencilState**   ppDepthStencilState) {
    InitReturnPtr(ppDepthStencilState);
    
    D3D10_DEPTH_STENCIL_DESC desc = pDepthStencilDesc != nullptr
      ? *pDepthStencilDesc
      : D3D10DepthStencilState::DefaultDesc();
    
    if (FAILED(D3D10DepthStencilState::NormalizeDesc(&desc)))
      return E_INVALIDARG;
    
    if (ppDepthStencilState != nullptr) {
      *ppDepthStencilState = m_dsStateObjects.Create(this, desc);
      return S_OK;
    } return S_FALSE;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateRasterizerState(
    const D3D10_RASTERIZER_DESC*      pRasterizerDesc,
          ID3D10RasterizerState**     ppRasterizerState) {
    InitReturnPtr(ppRasterizerState);
    
    D3D10_RASTERIZER_DESC desc = pRasterizerDesc != nullptr
      ? *pRasterizerDesc
      : D3D10RasterizerState::DefaultDesc();
    
    if (FAILED(D3D10RasterizerState::NormalizeDesc(&desc)))
      return E_INVALIDARG;
    
    if (ppRasterizerState != nullptr) {
      *ppRasterizerState = m_rsStateObjects.Create(this, desc);
      return S_OK;
    } return S_FALSE;
  }
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateSamplerState(
    const D3D10_SAMPLER_DESC*         pSamplerDesc,
          ID3D10SamplerState**        ppSamplerState) {
    InitReturnPtr(ppSamplerState);
    D3D10_SAMPLER_DESC desc = *pSamplerDesc;
    
    if (FAILED(D3D10SamplerState::NormalizeDesc(&desc)))
      return E_INVALIDARG;
    
    if (ppSamplerState == nullptr)
      return S_FALSE;
    
    try {
      *ppSamplerState = m_samplerObjects.Create(this, desc);
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateQuery(
    const D3D10_QUERY_DESC*           pQueryDesc,
          ID3D10Query**               ppQuery) {
    InitReturnPtr(ppQuery);
    
    if (pQueryDesc->Query != D3D10_QUERY_EVENT
     && pQueryDesc->Query != D3D10_QUERY_OCCLUSION
     && pQueryDesc->Query != D3D10_QUERY_TIMESTAMP
     && pQueryDesc->Query != D3D10_QUERY_TIMESTAMP_DISJOINT
     && pQueryDesc->Query != D3D10_QUERY_PIPELINE_STATISTICS
     && pQueryDesc->Query != D3D10_QUERY_OCCLUSION_PREDICATE) {
      Logger::warn(str::format("D3D10Query: Unsupported query type ", pQueryDesc->Query));
      return E_INVALIDARG;
    }
    
    if (ppQuery == nullptr)
      return S_FALSE;
    
    try {
      *ppQuery = ref(new D3D10Query(this, *pQueryDesc));
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreatePredicate(
    const D3D10_QUERY_DESC*           pPredicateDesc,
          ID3D10Predicate**           ppPredicate) {
    InitReturnPtr(ppPredicate);
    
    if (pPredicateDesc->Query != D3D10_QUERY_OCCLUSION_PREDICATE)
      return E_INVALIDARG;
    
    if (ppPredicate == nullptr)
      return S_FALSE;
    
    try {
      *ppPredicate = ref(new D3D10Query(this, *pPredicateDesc));
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CreateCounter(
    const D3D10_COUNTER_DESC*         pCounterDesc,
          ID3D10Counter**             ppCounter) {
    InitReturnPtr(ppCounter);
    
    Logger::err("D3D10Device::CreateCounter: Not implemented");
    return E_NOTIMPL;
  }
  
  HRESULT STDMETHODCALLTYPE D3D10Device::OpenSharedResource(
          HANDLE      hResource,
          REFIID      ReturnedInterface,
          void**      ppResource) {
    InitReturnPtr(ppResource);
    
    Logger::err("D3D10Device::OpenSharedResource: Not implemented");
    return E_NOTIMPL;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::OpenSharedResource1(
          HANDLE      hResource,
          REFIID      ReturnedInterface,
          void**      ppResource) {
    InitReturnPtr(ppResource);
    
    Logger::err("D3D10Device::OpenSharedResource1: Not implemented");
    return E_NOTIMPL;
  }

  
  HRESULT STDMETHODCALLTYPE D3D10Device::OpenSharedResourceByName(
          LPCWSTR     lpName, 
          DWORD       dwDesiredAccess, 
          REFIID      returnedInterface, 
          void**      ppResource) {
    InitReturnPtr(ppResource);
    
    Logger::err("D3D10Device::OpenSharedResourceByName: Not implemented");
    return E_NOTIMPL;
  }
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CheckFormatSupport(
          DXGI_FORMAT Format,
          UINT*       pFormatSupport) {
    return GetFormatSupportFlags(Format, pFormatSupport, nullptr);
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CheckMultisampleQualityLevels(
          DXGI_FORMAT Format,
          UINT        SampleCount,
          UINT*       pNumQualityLevels) {
    // There are many error conditions, so we'll just assume
    // that we will fail and return a non-zero value in case
    // the device does actually support the format.
    *pNumQualityLevels = 0;
    
    // We need to check whether the format is 
    VkFormat format = m_dxgiAdapter->LookupFormat(
      Format, DXGI_VK_FORMAT_MODE_ANY).Format;
    
    if (format == VK_FORMAT_UNDEFINED) {
      Logger::err(str::format("D3D10: Unsupported format: ", Format));
      return E_INVALIDARG;
    }
    
    // D3D may legally query non-power-of-two sample counts as well
    VkSampleCountFlagBits sampleCountFlag = VK_SAMPLE_COUNT_1_BIT;
    
    if (FAILED(DecodeSampleCount(SampleCount, &sampleCountFlag)))
      return E_INVALIDARG;
    
    // Check if the device supports the given combination of format
    // and sample count. D3D exposes the opaque concept of quality
    // levels to the application, we'll just define one such level.
    VkImageFormatProperties formatProps;
    
    VkResult status = m_dxvkAdapter->imageFormatProperties(
      format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT, 0, formatProps);
    
    if ((status == VK_SUCCESS) && (formatProps.sampleCounts & sampleCountFlag))
      *pNumQualityLevels = 1;
    return S_OK;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::CheckCounterInfo(D3D10_COUNTER_INFO* pCounterInfo) {
    Logger::err("D3D10Device::CheckCounterInfo: Not implemented");
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::CheckCounter(
    const D3D10_COUNTER_DESC* pDesc,
          D3D10_COUNTER_TYPE* pType,
          UINT*               pActiveCounters,
          LPSTR               szName,
          UINT*               pNameLength,
          LPSTR               szUnits,
          UINT*               pUnitsLength,
          LPSTR               szDescription,
          UINT*               pDescriptionLength) {
    Logger::err("D3D10Device::CheckCounter: Not implemented");
    return E_NOTIMPL;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::GetPrivateData(
          REFGUID guid, UINT* pDataSize, void* pData) {
    return m_container->GetPrivateData(guid, pDataSize, pData);
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::SetPrivateData(
          REFGUID guid, UINT DataSize, const void* pData) {
    return m_container->SetPrivateData(guid, DataSize, pData);
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::SetPrivateDataInterface(
          REFGUID guid, const IUnknown* pData) {
    return m_container->SetPrivateDataInterface(guid, pData);
  }
  
  
  D3D10_FEATURE_LEVEL1 STDMETHODCALLTYPE D3D10Device::GetFeatureLevel() {
    return m_featureLevel;
  }
  
  
  UINT STDMETHODCALLTYPE D3D10Device::GetCreationFlags() {
    return m_featureFlags;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Device::GetDeviceRemovedReason() {
    static std::atomic<bool> s_errorShown = { false };
    
    if (!s_errorShown.exchange(true))
      Logger::warn("D3D10Device::GetDeviceRemovedReason: Stub");
    
    return S_OK;
  }
  
  HRESULT STDMETHODCALLTYPE D3D10Device::SetExceptionMode(UINT RaiseFlags) {
    Logger::err("D3D10Device::SetExceptionMode: Not implemented");
    return E_NOTIMPL;
  }
  
  
  UINT STDMETHODCALLTYPE D3D10Device::GetExceptionMode() {
    Logger::err("D3D10Device::GetExceptionMode: Not implemented");
    return 0;
  }
  
  
  DXGI_VK_FORMAT_INFO STDMETHODCALLTYPE D3D10Device::LookupFormat(
          DXGI_FORMAT           format,
          DXGI_VK_FORMAT_MODE   mode) const {
    return m_dxgiAdapter->LookupFormat(format, mode);
  }
  
  
  VkPipelineStageFlags D3D10Device::GetEnabledShaderStages() const {
    VkPipelineStageFlags enabledShaderPipelineStages
      = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
      | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
      | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    
    if (m_dxvkDevice->features().geometryShader)
      enabledShaderPipelineStages |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
    
    if (m_dxvkDevice->features().tessellationShader) {
      enabledShaderPipelineStages |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
                                  |  VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    }
    
    return enabledShaderPipelineStages;
  }
  
  
  bool D3D10Device::CheckFeatureLevelSupport(
    const Rc<DxvkAdapter>&  adapter,
          D3D10_FEATURE_LEVEL1 featureLevel) {
    // We currently only support 11_0 interfaces
    if (featureLevel > GetMaxFeatureLevel())
      return false;
    
    // Check whether all features are supported
    const VkPhysicalDeviceFeatures features
      = GetDeviceFeatures(adapter, featureLevel);
    
    if (!adapter->checkFeatureSupport(features))
      return false;
    
    // TODO also check for required limits
    return true;
  }
  
  
  VkPhysicalDeviceFeatures D3D10Device::GetDeviceFeatures(
    const Rc<DxvkAdapter>&  adapter,
          D3D10_FEATURE_LEVEL1 featureLevel) {
    VkPhysicalDeviceFeatures supported = adapter->features();
    VkPhysicalDeviceFeatures enabled   = {};
    
    if (featureLevel >= D3D10_FEATURE_LEVEL_9_1) {
      enabled.depthClamp                            = VK_TRUE;
      enabled.depthBiasClamp                        = VK_TRUE;
      enabled.fillModeNonSolid                      = VK_TRUE;
      enabled.pipelineStatisticsQuery               = supported.pipelineStatisticsQuery;
      enabled.sampleRateShading                     = VK_TRUE;
      enabled.samplerAnisotropy                     = VK_TRUE;
      enabled.shaderClipDistance                    = VK_TRUE;
      enabled.shaderCullDistance                    = VK_TRUE;
      enabled.robustBufferAccess                    = VK_TRUE;
    }
    
    if (featureLevel >= D3D10_FEATURE_LEVEL_9_2) {
      enabled.occlusionQueryPrecise                 = VK_TRUE;
    }
    
    if (featureLevel >= D3D10_FEATURE_LEVEL_9_3) {
      enabled.multiViewport                         = VK_TRUE;
      enabled.independentBlend                      = VK_TRUE;
    }
    
    if (featureLevel >= D3D10_FEATURE_LEVEL_10_0) {
      enabled.fullDrawIndexUint32                   = VK_TRUE;
      enabled.fragmentStoresAndAtomics              = VK_TRUE;
      enabled.geometryShader                        = VK_TRUE;
      enabled.logicOp                               = supported.logicOp;
      enabled.shaderImageGatherExtended             = VK_TRUE;
      enabled.textureCompressionBC                  = VK_TRUE;
    }
    
    if (featureLevel >= D3D10_FEATURE_LEVEL_10_1) {
      enabled.dualSrcBlend                          = VK_TRUE;
      enabled.imageCubeArray                        = VK_TRUE;
    }
    
    return enabled;
  }
  
  
  HRESULT D3D10Device::CreateShaderModule(
          D3D10ShaderModule*      pShaderModule,
    const void*                   pShaderBytecode,
          size_t                  BytecodeLength,
          DxbcProgramType         ProgramType) {    
    try {
      *pShaderModule = m_shaderModules.GetShaderModule(
        &m_dxbcOptions, pShaderBytecode, BytecodeLength, ProgramType);
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  void D3D10Device::InitBuffer(
          D3D10Buffer*                pBuffer,
    const D3D10_SUBRESOURCE_DATA*     pInitialData) {
    const DxvkBufferSlice bufferSlice
      = pBuffer->GetBufferSlice();
    
    if (pInitialData != nullptr && pInitialData->pSysMem != nullptr) {
      LockResourceInitContext();
      
      m_resourceInitContext->updateBuffer(
        bufferSlice.buffer(),
        bufferSlice.offset(),
        bufferSlice.length(),
        pInitialData->pSysMem);
      
      UnlockResourceInitContext(1);
    }
  }
  
  
  void D3D10Device::InitTexture(
    const Rc<DxvkImage>&              image,
    const D3D10_SUBRESOURCE_DATA*     pInitialData) {
    const DxvkFormatInfo* formatInfo = imageFormatInfo(image->info().format);
    
    if (pInitialData != nullptr && pInitialData->pSysMem != nullptr) {
      LockResourceInitContext();
      
      // pInitialData is an array that stores an entry for
      // every single subresource. Since we will define all
      // subresources, this counts as initialization.
      VkImageSubresourceLayers subresourceLayers;
      subresourceLayers.aspectMask     = formatInfo->aspectMask;
      subresourceLayers.mipLevel       = 0;
      subresourceLayers.baseArrayLayer = 0;
      subresourceLayers.layerCount     = 1;
      
      for (uint32_t layer = 0; layer < image->info().numLayers; layer++) {
        for (uint32_t level = 0; level < image->info().mipLevels; level++) {
          subresourceLayers.baseArrayLayer = layer;
          subresourceLayers.mipLevel       = level;
          
          const uint32_t id = D3D10CalcSubresource(
            level, layer, image->info().mipLevels);
          
          m_resourceInitContext->updateImage(
            image, subresourceLayers,
            VkOffset3D { 0, 0, 0 },
            image->mipLevelExtent(level),
            pInitialData[id].pSysMem,
            pInitialData[id].SysMemPitch,
            pInitialData[id].SysMemSlicePitch);
        }
      }
      
      const uint32_t subresourceCount =
        image->info().numLayers * image->info().mipLevels;
      UnlockResourceInitContext(subresourceCount);
    } else {
      LockResourceInitContext();
      
      // While the Microsoft docs state that resource contents are
      // undefined if no initial data is provided, some applications
      // expect a resource to be pre-cleared. We can only do that
      // for non-compressed images, but that should be fine.
      VkImageSubresourceRange subresources;
      subresources.aspectMask     = formatInfo->aspectMask;
      subresources.baseMipLevel   = 0;
      subresources.levelCount     = image->info().mipLevels;
      subresources.baseArrayLayer = 0;
      subresources.layerCount     = image->info().numLayers;
      
      if (formatInfo->flags.test(DxvkFormatFlag::BlockCompressed)) {
        m_resourceInitContext->initImage(
          image, subresources);
      } else {
        if (subresources.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
          m_resourceInitContext->clearColorImage(
            image, VkClearColorValue(), subresources);
        } else {
          VkClearDepthStencilValue value;
          value.depth   = 1.0f;
          value.stencil = 0;
          
          m_resourceInitContext->clearDepthStencilImage(
            image, value, subresources);
        }
      }
      
      UnlockResourceInitContext(1);
    }
  }
  
  
  HRESULT D3D10Device::GetFormatSupportFlags(DXGI_FORMAT Format, UINT* pFlags1, UINT* pFlags2) const {
    // Query some general information from DXGI, DXVK and Vulkan about the format
    const DXGI_VK_FORMAT_INFO fmtMapping = m_dxgiAdapter->LookupFormat(Format, DXGI_VK_FORMAT_MODE_ANY);
    const VkFormatProperties  fmtSupport = m_dxvkAdapter->formatProperties(fmtMapping.Format);
    const DxvkFormatInfo*     fmtProperties = imageFormatInfo(fmtMapping.Format);
    
    // Reset output flags preemptively
    if (pFlags1 != nullptr) *pFlags1 = 0;
    if (pFlags2 != nullptr) *pFlags2 = 0;
    
    // Unsupported or invalid format
    if (fmtMapping.Format == VK_FORMAT_UNDEFINED)
      return E_INVALIDARG;
    
    UINT flags1 = 0;
    UINT flags2 = 0;
    
    // Format can be used for shader resource views with buffers
    if (fmtSupport.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)
      flags1 |= D3D10_FORMAT_SUPPORT_BUFFER;
    
    // Format can be used for vertex data
    if (fmtSupport.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
      flags1 |= D3D10_FORMAT_SUPPORT_IA_VERTEX_BUFFER;
    
    // Format can be used for index data. Only
    // these two formats are supported by D3D10.
    if (Format == DXGI_FORMAT_R16_UINT
     || Format == DXGI_FORMAT_R32_UINT)
      flags1 |= D3D10_FORMAT_SUPPORT_IA_INDEX_BUFFER;
    
    // TODO implement stream output
    // D3D10_FORMAT_SUPPORT_SO_BUFFER
    
    if (fmtSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
     || fmtSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) {
      const VkFormat depthFormat = m_dxgiAdapter->LookupFormat(Format, DXGI_VK_FORMAT_MODE_DEPTH).Format;
      
      if (GetImageTypeSupport(fmtMapping.Format, VK_IMAGE_TYPE_1D)) flags1 |= D3D10_FORMAT_SUPPORT_TEXTURE1D;
      if (GetImageTypeSupport(fmtMapping.Format, VK_IMAGE_TYPE_2D)) flags1 |= D3D10_FORMAT_SUPPORT_TEXTURE2D;
      if (GetImageTypeSupport(fmtMapping.Format, VK_IMAGE_TYPE_3D)) flags1 |= D3D10_FORMAT_SUPPORT_TEXTURE3D;
      
      flags1 |= D3D10_FORMAT_SUPPORT_MIP
             |  D3D10_FORMAT_SUPPORT_CPU_LOCKABLE
             |  D3D10_FORMAT_SUPPORT_CAST_WITHIN_BIT_LAYOUT;
    
      // Format can be read 
      if (fmtSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
        flags1 |= D3D10_FORMAT_SUPPORT_TEXTURECUBE
               |  D3D10_FORMAT_SUPPORT_SHADER_LOAD
               |  D3D10_FORMAT_SUPPORT_SHADER_GATHER
               |  D3D10_FORMAT_SUPPORT_SHADER_SAMPLE;
        
        if (depthFormat != VK_FORMAT_UNDEFINED) {
          flags1 |= D3D10_FORMAT_SUPPORT_SHADER_SAMPLE_COMPARISON;
        }
      }
      
      // Format is a color format that can be used for rendering
      if (fmtSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) {
        flags1 |= D3D10_FORMAT_SUPPORT_RENDER_TARGET
               |  D3D10_FORMAT_SUPPORT_MIP_AUTOGEN;
      }
      
      // Format supports blending when used for rendering
      if (fmtSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
        flags1 |= D3D10_FORMAT_SUPPORT_BLENDABLE;
      
      // Format is a depth-stencil format that can be used for rendering
      if (fmtSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        flags1 |= D3D10_FORMAT_SUPPORT_DEPTH_STENCIL;
      
      // FIXME implement properly. This would require a VkSurface.
      if (Format == DXGI_FORMAT_R8G8B8A8_UNORM
       || Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
       || Format == DXGI_FORMAT_B8G8R8A8_UNORM
       || Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
       || Format == DXGI_FORMAT_R16G16B16A16_FLOAT
       || Format == DXGI_FORMAT_R10G10B10A2_UNORM
       || Format == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
        flags1 |= D3D10_FORMAT_SUPPORT_DISPLAY;
      
      // Query multisample support for this format
      VkImageFormatProperties imgFmtProperties;
      
      VkResult status = m_dxvkAdapter->imageFormatProperties(fmtMapping.Format,
        VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
        (fmtProperties->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
          ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
          : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        0, imgFmtProperties);
      
      if (status == VK_SUCCESS && imgFmtProperties.sampleCounts > VK_SAMPLE_COUNT_1_BIT) {
        flags1 |= D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET
               |  D3D10_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE
               |  D3D10_FORMAT_SUPPORT_MULTISAMPLE_LOAD;
      }
    }
    
    // Write back format support flags
    if (pFlags1 != nullptr) *pFlags1 = flags1;
    if (pFlags2 != nullptr) *pFlags2 = flags2;
    return S_OK;
  }
  
  
  BOOL D3D10Device::GetImageTypeSupport(VkFormat Format, VkImageType Type) const {
    VkImageFormatProperties props;
    
    VkResult status = m_dxvkAdapter->imageFormatProperties(
      Format, Type, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT, 0, props);
    
    return status == VK_SUCCESS;
  }
  
  
  void D3D10Device::LockResourceInitContext() {
    m_resourceInitMutex.lock();
  }
  
  
  void D3D10Device::UnlockResourceInitContext(uint64_t CommandCount) {
    m_resourceInitCommands += CommandCount;
    
    if (m_resourceInitCommands >= InitCommandThreshold)
      SubmitResourceInitCommands();
    
    m_resourceInitMutex.unlock();
  }
  
  
  void D3D10Device::SubmitResourceInitCommands() {
    m_dxvkDevice->submitCommandList(
      m_resourceInitContext->endRecording(),
      nullptr, nullptr);
    
    m_resourceInitContext->beginRecording(
      m_dxvkDevice->createCommandList());
    
    m_resourceInitCommands = 0;
  }
  
  
  D3D10_FEATURE_LEVEL1 D3D10Device::GetMaxFeatureLevel() {
    static const std::array<std::pair<std::string, D3D10_FEATURE_LEVEL1>, 5> s_featureLevels = {{
      { "10_1", D3D10_FEATURE_LEVEL_10_1 },
      { "10_0", D3D10_FEATURE_LEVEL_10_0 },
      { "9_3",  D3D10_FEATURE_LEVEL_9_3  },
      { "9_2",  D3D10_FEATURE_LEVEL_9_2  },
      { "9_1",  D3D10_FEATURE_LEVEL_9_1  },
    }};
    
    const std::string maxLevel = env::getEnvVar(L"DXVK_FEATURE_LEVEL");
    
    auto entry = std::find_if(s_featureLevels.begin(), s_featureLevels.end(),
      [&] (const std::pair<std::string, D3D10_FEATURE_LEVEL1>& pair) {
        return pair.first == maxLevel;
      });
    
    return entry != s_featureLevels.end()
      ? entry->second
      : D3D10_FEATURE_LEVEL_10_0;
  }
  
}
