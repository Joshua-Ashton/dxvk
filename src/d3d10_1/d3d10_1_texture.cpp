#include "d3d10_1_device.h"
#include "d3d10_1_texture.h"

namespace dxvk {
  
  D3D10CommonTexture::D3D10CommonTexture(
          D3D10Device*                pDevice,
    const D3D10_COMMON_TEXTURE_DESC*  pDesc,
          D3D10_RESOURCE_DIMENSION    Dimension)
  : m_device(pDevice), m_desc(*pDesc) {
    DXGI_VK_FORMAT_INFO formatInfo = m_device->LookupFormat(m_desc.Format, GetFormatMode());
    
    DxvkImageCreateInfo imageInfo;
    imageInfo.type           = GetImageTypeFromResourceDim(Dimension);
    imageInfo.format         = formatInfo.Format;
    imageInfo.flags          = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    imageInfo.sampleCount    = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.extent.width   = m_desc.Width;
    imageInfo.extent.height  = m_desc.Height;
    imageInfo.extent.depth   = m_desc.Depth;
    imageInfo.numLayers      = m_desc.ArraySize;
    imageInfo.mipLevels      = m_desc.MipLevels;
    imageInfo.usage          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                             | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.stages         = VK_PIPELINE_STAGE_TRANSFER_BIT;
    imageInfo.access         = VK_ACCESS_TRANSFER_READ_BIT
                             | VK_ACCESS_TRANSFER_WRITE_BIT;
    imageInfo.tiling         = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.layout         = VK_IMAGE_LAYOUT_GENERAL;
    
    DecodeSampleCount(m_desc.SampleDesc.Count, &imageInfo.sampleCount);
    
    // Adjust image flags based on the corresponding D3D flags
    if (m_desc.BindFlags & D3D10_BIND_SHADER_RESOURCE) {
      imageInfo.usage  |= VK_IMAGE_USAGE_SAMPLED_BIT;
      imageInfo.stages |= pDevice->GetEnabledShaderStages();
      imageInfo.access |= VK_ACCESS_SHADER_READ_BIT;
    }
    
    if (m_desc.BindFlags & D3D10_BIND_RENDER_TARGET) {
      imageInfo.usage  |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      imageInfo.stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      imageInfo.access |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                       |  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    
    if (m_desc.BindFlags & D3D10_BIND_DEPTH_STENCIL) {
      imageInfo.usage  |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      imageInfo.stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                       |  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      imageInfo.access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                       |  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    
    if (m_desc.MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE)
      imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    if (Dimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D)
      imageInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
    
    // Test whether the combination of image parameters is supported
    if (!CheckImageSupport(&imageInfo, VK_IMAGE_TILING_OPTIMAL)) {
      throw DxvkError(str::format(
        "D3D10: Cannot create texture:",
        "\n  Format:  ", imageInfo.format,
        "\n  Extent:  ", imageInfo.extent.width,
                    "x", imageInfo.extent.height,
                    "x", imageInfo.extent.depth,
        "\n  Samples: ", imageInfo.sampleCount,
        "\n  Layers:  ", imageInfo.numLayers,
        "\n  Levels:  ", imageInfo.mipLevels,
        "\n  Usage:   ", std::hex, imageInfo.usage));
    }
    
    // Determine map mode based on our findings
    m_mapMode = DetermineMapMode(&imageInfo);
    
    // If the image is mapped directly to host memory, we need
    // to enable linear tiling, and DXVK needs to be aware that
    // the image can be accessed by the host.
    if (m_mapMode == D3D10_COMMON_TEXTURE_MAP_MODE_DIRECT) {
      imageInfo.stages |= VK_PIPELINE_STAGE_HOST_BIT;
      imageInfo.tiling  = VK_IMAGE_TILING_LINEAR;
      
      if (m_desc.CPUAccessFlags & D3D10_CPU_ACCESS_WRITE)
        imageInfo.access |= VK_ACCESS_HOST_WRITE_BIT;
      
      if (m_desc.CPUAccessFlags & D3D10_CPU_ACCESS_READ)
        imageInfo.access |= VK_ACCESS_HOST_READ_BIT;
    }
    
    // We must keep LINEAR images in GENERAL layout, but we
    // can choose a better layout for the image based on how
    // it is going to be used by the game.
    if (imageInfo.tiling == VK_IMAGE_TILING_OPTIMAL)
      imageInfo.layout = OptimizeLayout(imageInfo.usage);
    
    // If necessary, create the mapped linear buffer
    if (m_mapMode == D3D10_COMMON_TEXTURE_MAP_MODE_BUFFER)
      m_buffer = CreateMappedBuffer();
    
    // Create the image on a host-visible memory type
    // in case it is going to be mapped directly.
    VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    if (m_mapMode == D3D10_COMMON_TEXTURE_MAP_MODE_DIRECT) {
      memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                       | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                       | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    }
    
    m_image = m_device->GetDXVKDevice()->createImage(imageInfo, memoryProperties);
  }
  
  
  D3D10CommonTexture::~D3D10CommonTexture() {
    
  }

  HRESULT D3D10CommonTexture::Map(UINT Subresource, D3D10_MAP MapType, UINT MapFlags, void * pMappedData)
  {
	  const Rc<DxvkImage>  mappedImage = GetImage();
	  const Rc<DxvkBuffer> mappedBuffer = GetMappedBuffer();

	  if (GetMapMode() == D3D10_COMMON_TEXTURE_MAP_MODE_NONE) {
		  Logger::err("D3D11: Cannot map a device-local image");
		  return E_INVALIDARG;
	  }

    auto formatInfo = imageFormatInfo(mappedImage->info().format);
    
    if (formatInfo->aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
      Logger::err("D3D11: Cannot map a depth-stencil texture");
      return E_INVALIDARG;
    }
	  
	  // Parameter validation was successful
	  VkImageSubresource subresource =
		  GetSubresourceFromIndex(
			  formatInfo->aspectMask, Subresource);

	  SetMappedSubresource(subresource);

	  const VkImageType imageType = mappedImage->info().type;
	  if (GetMapMode() == D3D10_COMMON_TEXTURE_MAP_MODE_DIRECT) {

		  // Wait for the resource to become available
		  if (!m_device->WaitForResource(mappedImage, MapFlags))
			  return DXGI_ERROR_WAS_STILL_DRAWING;

		  // Query the subresource's memory layout and hope that
		  // the application respects the returned pitch values.
		  VkSubresourceLayout layout = mappedImage->querySubresourceLayout(subresource);

		  if (pMappedData)
		  {
			  switch (imageType)
			  {
			  case VK_IMAGE_TYPE_1D:
			  {
				  void** pData = (void**)pMappedData;
				  *pData = mappedImage->mapPtr(layout.offset);
			  } break;

			  default:
			  case VK_IMAGE_TYPE_2D:
			  {
				  D3D10_MAPPED_TEXTURE2D* pMappedResource = (D3D10_MAPPED_TEXTURE2D*)pMappedData;
				  pMappedResource->pData = mappedImage->mapPtr(layout.offset);
				  pMappedResource->RowPitch = layout.rowPitch;
			  } break;

			  case VK_IMAGE_TYPE_3D:
			  {
				  D3D10_MAPPED_TEXTURE3D* pMappedResource = (D3D10_MAPPED_TEXTURE3D*)pMappedData;
				  pMappedResource->pData = mappedImage->mapPtr(layout.offset);
				  pMappedResource->RowPitch = layout.rowPitch;
			  } break;

			  }
		  }
		  return S_OK;
	  }
	  else {
		  // Query format info which we need to compute
		  // the row pitch and layer pitch properly.
		  const VkExtent3D levelExtent = mappedImage->mipLevelExtent(subresource.mipLevel);
		  const VkExtent3D blockCount = util::computeBlockCount(levelExtent, formatInfo->blockSize);

		  DxvkPhysicalBufferSlice physicalSlice;

		  if (MapType == D3D10_MAP_WRITE_DISCARD) {
			  // We do not have to preserve the contents of the
			  // buffer if the entire image gets discarded.
			  physicalSlice = mappedBuffer->allocPhysicalSlice();

			  m_device->EmitCs([
				  cImageBuffer = mappedBuffer,
					  cPhysicalSlice = physicalSlice
			  ] (DxvkContext* ctx) {
				  ctx->invalidateBuffer(cImageBuffer, cPhysicalSlice);
			  });
		  }
		  else {
			  // When using any map mode which requires the image contents
			  // to be preserved, and if the GPU has write access to the
			  // image, copy the current image contents into the buffer.
			  const bool copyExistingData = Desc()->Usage == D3D10_USAGE_STAGING;

			  if (copyExistingData) {
				  const VkImageSubresourceLayers subresourceLayers = {
					  subresource.aspectMask,
					  subresource.mipLevel,
					  subresource.arrayLayer, 1 };

				  m_device->EmitCs([
					  cImageBuffer = mappedBuffer,
						  cImage = mappedImage,
						  cSubresources = subresourceLayers,
						  cLevelExtent = levelExtent
				  ] (DxvkContext* ctx) {
					  ctx->copyImageToBuffer(
						  cImageBuffer, 0, VkExtent2D{ 0u, 0u },
						  cImage, cSubresources, VkOffset3D{ 0, 0, 0 },
						  cLevelExtent);
				  });
			  }

			  m_device->WaitForResource(mappedBuffer->resource(), 0);
			  physicalSlice = mappedBuffer->slice();
		  }

		  if (pMappedData)
		  {
			  switch (imageType)
			  {
			  case VK_IMAGE_TYPE_1D:
			  {
				  void** pData = (void**)pMappedData;
				  *pData = physicalSlice.mapPtr(0);
			  } break;

			  default:
			  case VK_IMAGE_TYPE_2D:
			  {
				  D3D10_MAPPED_TEXTURE2D* pMappedResource = (D3D10_MAPPED_TEXTURE2D*)pMappedData;
				  pMappedResource->pData = physicalSlice.mapPtr(0);
				  pMappedResource->RowPitch = formatInfo->elementSize * blockCount.width;
			  } break;

			  case VK_IMAGE_TYPE_3D:
			  {
				  D3D10_MAPPED_TEXTURE3D* pMappedResource = (D3D10_MAPPED_TEXTURE3D*)pMappedData;
				  pMappedResource->pData = physicalSlice.mapPtr(0);
				  pMappedResource->RowPitch = formatInfo->elementSize * blockCount.width;
			  } break;

			  }
		  }
		  return S_OK;
	  }
  }

  void D3D10CommonTexture::Unmap(UINT Subresource)
  {
	  if (GetMapMode() == D3D10_COMMON_TEXTURE_MAP_MODE_BUFFER) {
		  // Now that data has been written into the buffer,
		  // we need to copy its contents into the image
		  const Rc<DxvkImage>  mappedImage = GetImage();
		  const Rc<DxvkBuffer> mappedBuffer = GetMappedBuffer();

		  VkImageSubresource subresource = GetMappedSubresource();

		  VkExtent3D levelExtent = mappedImage
			  ->mipLevelExtent(subresource.mipLevel);

		  VkImageSubresourceLayers subresourceLayers = {
			  subresource.aspectMask,
			  subresource.mipLevel,
			  subresource.arrayLayer, 1 };

		  m_device->EmitCs([
			  cSrcBuffer = mappedBuffer,
				  cDstImage = mappedImage,
				  cDstLayers = subresourceLayers,
				  cDstLevelExtent = levelExtent
		  ] (DxvkContext* ctx) {
			  ctx->copyBufferToImage(cDstImage, cDstLayers,
				  VkOffset3D{ 0, 0, 0 }, cDstLevelExtent,
				  cSrcBuffer, 0, { 0u, 0u });
		  });
	  }

	 ClearMappedSubresource();
  }
  
  
  VkImageSubresource D3D10CommonTexture::GetSubresourceFromIndex(
          VkImageAspectFlags    Aspect,
          UINT                  Subresource) const {
    VkImageSubresource result;
    result.aspectMask     = Aspect;
    result.mipLevel       = Subresource % m_desc.MipLevels;
    result.arrayLayer     = Subresource / m_desc.MipLevels;
    return result;
  }
  
  
  DXGI_VK_FORMAT_MODE D3D10CommonTexture::GetFormatMode() const {
    if (m_desc.BindFlags & D3D10_BIND_RENDER_TARGET)
      return DXGI_VK_FORMAT_MODE_COLOR;
    
    if (m_desc.BindFlags & D3D10_BIND_DEPTH_STENCIL)
      return DXGI_VK_FORMAT_MODE_DEPTH;
    
    return DXGI_VK_FORMAT_MODE_ANY;
  }
  
  
  void D3D10CommonTexture::GetDevice(ID3D10Device** ppDevice) const {
    *ppDevice = m_device.ref();
  }
  
  
  HRESULT D3D10CommonTexture::NormalizeTextureProperties(D3D10_COMMON_TEXTURE_DESC* pDesc) {
    if (FAILED(DecodeSampleCount(pDesc->SampleDesc.Count, nullptr)))
      return E_INVALIDARG;
    
    // Use the maximum possible mip level count if the supplied
    // mip level count is either unspecified (0) or invalid
    const uint32_t maxMipLevelCount = pDesc->SampleDesc.Count <= 1
      ? util::computeMipLevelCount({ pDesc->Width, pDesc->Height, pDesc->Depth })
      : 1u;
    
    if (pDesc->MipLevels == 0 || pDesc->MipLevels > maxMipLevelCount)
      pDesc->MipLevels = maxMipLevelCount;
    
    return S_OK;
  }
  
  BOOL D3D10CommonTexture::CheckImageSupport(
    const DxvkImageCreateInfo*  pImageInfo,
          VkImageTiling         Tiling) const {
    const Rc<DxvkAdapter> adapter = m_device->GetDXVKDevice()->adapter();
    
    VkImageFormatProperties formatProps = { };
    
    VkResult status = adapter->imageFormatProperties(
      pImageInfo->format, pImageInfo->type, Tiling,
      pImageInfo->usage, pImageInfo->flags, formatProps);
    
    if (status != VK_SUCCESS)
      return FALSE;
    
    return (pImageInfo->extent.width  <= formatProps.maxExtent.width)
        && (pImageInfo->extent.height <= formatProps.maxExtent.height)
        && (pImageInfo->extent.depth  <= formatProps.maxExtent.depth)
        && (pImageInfo->numLayers     <= formatProps.maxArrayLayers)
        && (pImageInfo->mipLevels     <= formatProps.maxMipLevels)
        && (pImageInfo->sampleCount    & formatProps.sampleCounts);
  }
  
  
  D3D10_COMMON_TEXTURE_MAP_MODE D3D10CommonTexture::DetermineMapMode(
    const DxvkImageCreateInfo*  pImageInfo) const {
    // Don't map an image unless the application requests it
    if (m_desc.CPUAccessFlags == 0)
      return D3D10_COMMON_TEXTURE_MAP_MODE_NONE;
    
    // Write-only images should go through a buffer for multiple reasons:
    // 1. Some games do not respect the row and depth pitch that is returned
    //    by the Map() method, which leads to incorrect rendering (e.g. Nier)
    // 2. Since the image will most likely be read for rendering by the GPU,
    //    writing the image to device-local image may be more efficient than
    //    reading its contents from host-visible memory.
    if (m_desc.Usage == D3D10_USAGE_DYNAMIC)
      return D3D10_COMMON_TEXTURE_MAP_MODE_BUFFER;
    
    // Images that can be read by the host should be mapped directly in
    // order to avoid expensive synchronization with the GPU. This does
    // however require linear tiling, which may not be supported for all
    // combinations of image parameters.
    return this->CheckImageSupport(pImageInfo, VK_IMAGE_TILING_LINEAR)
      ? D3D10_COMMON_TEXTURE_MAP_MODE_DIRECT
      : D3D10_COMMON_TEXTURE_MAP_MODE_BUFFER;
  }
  
  
  Rc<DxvkBuffer> D3D10CommonTexture::CreateMappedBuffer() const {
    const DxvkFormatInfo* formatInfo = imageFormatInfo(
      m_device->LookupFormat(m_desc.Format, GetFormatMode()).Format);
    
    const VkExtent3D blockCount = util::computeBlockCount(
      VkExtent3D { m_desc.Width, m_desc.Height, m_desc.Depth },
      formatInfo->blockSize);
    
    DxvkBufferCreateInfo info;
    info.size   = formatInfo->elementSize
                * blockCount.width
                * blockCount.height
                * blockCount.depth;
    info.usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
    info.access = VK_ACCESS_TRANSFER_READ_BIT
                | VK_ACCESS_TRANSFER_WRITE_BIT;
    
    return m_device->GetDXVKDevice()->createBuffer(info,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }
  
  
  VkImageType D3D10CommonTexture::GetImageTypeFromResourceDim(D3D10_RESOURCE_DIMENSION Dimension) {
    switch (Dimension) {
      case D3D10_RESOURCE_DIMENSION_TEXTURE1D: return VK_IMAGE_TYPE_1D;
      case D3D10_RESOURCE_DIMENSION_TEXTURE2D: return VK_IMAGE_TYPE_2D;
      case D3D10_RESOURCE_DIMENSION_TEXTURE3D: return VK_IMAGE_TYPE_3D;
      default: throw DxvkError("D3D10CommonTexture: Unhandled resource dimension");
    }
  }
  
  
  VkImageLayout D3D10CommonTexture::OptimizeLayout(VkImageUsageFlags Usage) {
    const VkImageUsageFlags usageFlags = Usage;
    
    // Filter out unnecessary flags. Transfer operations
    // are handled by the backend in a transparent manner.
    Usage &= ~(VK_IMAGE_USAGE_TRANSFER_DST_BIT
             | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    
    // If the image is used only as an attachment, we never
    // have to transform the image back to a different layout
    if (Usage == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    if (Usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    Usage &= ~(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
             | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    
    // If the image is used for reading but not as a storage
    // image, we can optimize the image for texture access
    if (Usage == VK_IMAGE_USAGE_SAMPLED_BIT) {
      return usageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    
    // Otherwise, we have to stick with the default layout
    return VK_IMAGE_LAYOUT_GENERAL;
  }
  
  
  ///////////////////////////////////////////
  //      D 3 D 1 1 T E X T U R E 1 D
  D3D10Texture1D::D3D10Texture1D(
          D3D10Device*                pDevice,
    const D3D10_COMMON_TEXTURE_DESC*  pDesc)
  : m_texture(pDevice, pDesc, D3D10_RESOURCE_DIMENSION_TEXTURE1D) {
    
  }
  
  
  D3D10Texture1D::~D3D10Texture1D() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Texture1D::QueryInterface(REFIID riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(ID3D10DeviceChild)
     || riid == __uuidof(ID3D10Resource)
     || riid == __uuidof(ID3D10Texture1D)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    Logger::warn("D3D10Texture1D::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  
    
  void STDMETHODCALLTYPE D3D10Texture1D::GetDevice(ID3D10Device** ppDevice) {
    m_texture.GetDevice(ppDevice);
  }
  
  
  void STDMETHODCALLTYPE D3D10Texture1D::GetType(D3D10_RESOURCE_DIMENSION *pResourceDimension) {
    *pResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE1D;
  }
  
  
  UINT STDMETHODCALLTYPE D3D10Texture1D::GetEvictionPriority() {
    Logger::warn("D3D10Texture1D::GetEvictionPriority: Stub");
    return DXGI_RESOURCE_PRIORITY_NORMAL;
  }
  
  
  void STDMETHODCALLTYPE D3D10Texture1D::SetEvictionPriority(UINT EvictionPriority) {
    Logger::warn("D3D10Texture1D::SetEvictionPriority: Stub");
  }
  
  
  void STDMETHODCALLTYPE D3D10Texture1D::GetDesc(D3D10_TEXTURE1D_DESC *pDesc) {
    pDesc->Width          = m_texture.Desc()->Width;
    pDesc->MipLevels      = m_texture.Desc()->MipLevels;
    pDesc->ArraySize      = m_texture.Desc()->ArraySize;
    pDesc->Format         = m_texture.Desc()->Format;
    pDesc->Usage          = m_texture.Desc()->Usage;
    pDesc->BindFlags      = m_texture.Desc()->BindFlags;
    pDesc->CPUAccessFlags = m_texture.Desc()->CPUAccessFlags;
    pDesc->MiscFlags      = m_texture.Desc()->MiscFlags;
  }
  
  
  ///////////////////////////////////////////
  //      D 3 D 1 1 T E X T U R E 2 D
  D3D10Texture2D::D3D10Texture2D(
          D3D10Device*                pDevice,
    const D3D10_COMMON_TEXTURE_DESC*  pDesc)
  : m_texture(pDevice, pDesc, D3D10_RESOURCE_DIMENSION_TEXTURE2D) {
    
  }
  
  
  D3D10Texture2D::~D3D10Texture2D() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Texture2D::QueryInterface(REFIID riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(ID3D10DeviceChild)
     || riid == __uuidof(ID3D10Resource)
     || riid == __uuidof(ID3D10Texture2D)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    Logger::warn("D3D10Texture2D::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  
    
  void STDMETHODCALLTYPE D3D10Texture2D::GetDevice(ID3D10Device** ppDevice) {
    m_texture.GetDevice(ppDevice);
  }
  
  
  void STDMETHODCALLTYPE D3D10Texture2D::GetType(D3D10_RESOURCE_DIMENSION *pResourceDimension) {
    *pResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
  }
  
  
  UINT STDMETHODCALLTYPE D3D10Texture2D::GetEvictionPriority() {
    Logger::warn("D3D10Texture2D::GetEvictionPriority: Stub");
    return DXGI_RESOURCE_PRIORITY_NORMAL;
  }
  
  
  void STDMETHODCALLTYPE D3D10Texture2D::SetEvictionPriority(UINT EvictionPriority) {
    Logger::warn("D3D10Texture2D::SetEvictionPriority: Stub");
  }
  
  
  void STDMETHODCALLTYPE D3D10Texture2D::GetDesc(D3D10_TEXTURE2D_DESC *pDesc) {
    pDesc->Width          = m_texture.Desc()->Width;
    pDesc->Height         = m_texture.Desc()->Height;
    pDesc->MipLevels      = m_texture.Desc()->MipLevels;
    pDesc->ArraySize      = m_texture.Desc()->ArraySize;
    pDesc->Format         = m_texture.Desc()->Format;
    pDesc->SampleDesc     = m_texture.Desc()->SampleDesc;
    pDesc->Usage          = m_texture.Desc()->Usage;
    pDesc->BindFlags      = m_texture.Desc()->BindFlags;
    pDesc->CPUAccessFlags = m_texture.Desc()->CPUAccessFlags;
    pDesc->MiscFlags      = m_texture.Desc()->MiscFlags;
  }
  
  
  ///////////////////////////////////////////
  //      D 3 D 1 1 T E X T U R E 3 D
  D3D10Texture3D::D3D10Texture3D(
          D3D10Device*                pDevice,
    const D3D10_COMMON_TEXTURE_DESC*  pDesc)
  : m_texture(pDevice, pDesc, D3D10_RESOURCE_DIMENSION_TEXTURE3D) {
    
  }
  
  
  D3D10Texture3D::~D3D10Texture3D() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Texture3D::QueryInterface(REFIID riid, void** ppvObject) {
    *ppvObject = nullptr;
    
    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(ID3D10DeviceChild)
     || riid == __uuidof(ID3D10Resource)
     || riid == __uuidof(ID3D10Texture3D)) {
      *ppvObject = ref(this);
      return S_OK;
    }
    
    Logger::warn("D3D10Texture3D::QueryInterface: Unknown interface query");
    Logger::warn(str::format(riid));
    return E_NOINTERFACE;
  }
  
    
  void STDMETHODCALLTYPE D3D10Texture3D::GetDevice(ID3D10Device** ppDevice) {
    m_texture.GetDevice(ppDevice);
  }
  
  
  void STDMETHODCALLTYPE D3D10Texture3D::GetType(D3D10_RESOURCE_DIMENSION *pResourceDimension) {
    *pResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE3D;
  }
  
  
  UINT STDMETHODCALLTYPE D3D10Texture3D::GetEvictionPriority() {
    Logger::warn("D3D10Texture3D::GetEvictionPriority: Stub");
    return DXGI_RESOURCE_PRIORITY_NORMAL;
  }
  
  
  void STDMETHODCALLTYPE D3D10Texture3D::SetEvictionPriority(UINT EvictionPriority) {
    Logger::warn("D3D10Texture3D::SetEvictionPriority: Stub");
  }
  
  
  void STDMETHODCALLTYPE D3D10Texture3D::GetDesc(D3D10_TEXTURE3D_DESC *pDesc) {
    pDesc->Width          = m_texture.Desc()->Width;
    pDesc->Height         = m_texture.Desc()->Height;
    pDesc->Depth          = m_texture.Desc()->Depth;
    pDesc->MipLevels      = m_texture.Desc()->MipLevels;
    pDesc->Format         = m_texture.Desc()->Format;
    pDesc->Usage          = m_texture.Desc()->Usage;
    pDesc->BindFlags      = m_texture.Desc()->BindFlags;
    pDesc->CPUAccessFlags = m_texture.Desc()->CPUAccessFlags;
    pDesc->MiscFlags      = m_texture.Desc()->MiscFlags;
  }
  
  
  
  D3D10CommonTexture* GetCommonTexture(ID3D10Resource* pResource) {
    D3D10_RESOURCE_DIMENSION dimension = D3D10_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&dimension);
    
    switch (dimension) {
      case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
        return static_cast<D3D10Texture1D*>(pResource)->GetCommonTexture();
      
      case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
        return static_cast<D3D10Texture2D*>(pResource)->GetCommonTexture();
      
      case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
        return static_cast<D3D10Texture3D*>(pResource)->GetCommonTexture();
      
      default:
        return nullptr;
    }
  }
  
  HRESULT STDMETHODCALLTYPE D3D10Texture1D::Map(
          UINT Subresource,
          D3D10_MAP MapType,
          UINT MapFlags,
          void **ppData)
  {
	  return m_texture.Map(Subresource, MapType, MapFlags, (void*)ppData);
  }

  void STDMETHODCALLTYPE D3D10Texture1D::Unmap(
          UINT Subresource)
  {
	  m_texture.Unmap(Subresource);
  }

  HRESULT STDMETHODCALLTYPE D3D10Texture2D::Map( 
          UINT Subresource,
          D3D10_MAP MapType,
          UINT MapFlags,
          D3D10_MAPPED_TEXTURE2D *pMappedTex2D)
  {
	  return m_texture.Map(Subresource, MapType, MapFlags, (void*)pMappedTex2D);
  }

    void STDMETHODCALLTYPE D3D10Texture2D::Unmap( 
              UINT Subresource)
    {
		m_texture.Unmap(Subresource);
    }

  HRESULT STDMETHODCALLTYPE D3D10Texture3D::Map( 
          UINT Subresource,
          D3D10_MAP MapType,
          UINT MapFlags,
          D3D10_MAPPED_TEXTURE3D *pMappedTex3D)
  {
	  return m_texture.Map(Subresource, MapType, MapFlags, (void*)pMappedTex3D);
  }

    void STDMETHODCALLTYPE D3D10Texture3D::Unmap( 
              UINT Subresource)
    {
		m_texture.Unmap(Subresource);
    }
}
