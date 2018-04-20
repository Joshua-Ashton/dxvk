#include <cstring>

#include "d3d10_1_device.h"
#include "d3d10_1_query.h"
#include "d3d10_1_texture.h"

#include "../dxbc/dxbc_util.h"

#include "d3d10_1_view_dsv.h"
#include "d3d10_1_view_rtv.h"
#include "d3d10_1_view_srv.h"

#include "d3d10_1_buffer.h"
#include "d3d10_1_input_layout.h"

namespace dxvk {
  
  void STDMETHODCALLTYPE D3D10Device::ClearState() {
    // Default shaders
    m_state.vs.shader = nullptr;
    m_state.gs.shader = nullptr;
    m_state.ps.shader = nullptr;
    
    // Default constant buffers
    for (uint32_t i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; i++) {
      m_state.vs.constantBuffers[i] = { nullptr, 0, 0 };
      m_state.gs.constantBuffers[i] = { nullptr, 0, 0 };
      m_state.ps.constantBuffers[i] = { nullptr, 0, 0 };
    }
    
    // Default samplers
    for (uint32_t i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; i++) {
      m_state.vs.samplers[i] = nullptr;
      m_state.gs.samplers[i] = nullptr;
	  m_state.ps.samplers[i] = nullptr;
    }
    
    // Default shader resources
    for (uint32_t i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++) {
      m_state.vs.shaderResources[i] = nullptr;
      m_state.gs.shaderResources[i] = nullptr;
      m_state.ps.shaderResources[i] = nullptr;
    }
    
    // Default IA state
    m_state.ia.inputLayout       = nullptr;
    m_state.ia.primitiveTopology = D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;
    
    for (uint32_t i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; i++) {
      m_state.ia.vertexBuffers[i].buffer = nullptr;
      m_state.ia.vertexBuffers[i].offset = 0;
      m_state.ia.vertexBuffers[i].stride = 0;
    }
    
    m_state.ia.indexBuffer.buffer = nullptr;
    m_state.ia.indexBuffer.offset = 0;
    m_state.ia.indexBuffer.format = DXGI_FORMAT_UNKNOWN;
    
    // Default OM State
    for (uint32_t i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
      m_state.om.renderTargetViews[i] = nullptr;
    m_state.om.depthStencilView = nullptr;
    
    m_state.om.cbState = nullptr;
    m_state.om.dsState = nullptr;
    
    for (uint32_t i = 0; i < 4; i++)
      m_state.om.blendFactor[i] = 0.0f;
    
    m_state.om.sampleMask = D3D10_DEFAULT_SAMPLE_MASK;
    m_state.om.stencilRef = D3D10_DEFAULT_STENCIL_REFERENCE;
    
    // Default RS state
    m_state.rs.state        = nullptr;
    m_state.rs.numViewports = 0;
    m_state.rs.numScissors  = 0;
    
    for (uint32_t i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; i++) {
      m_state.rs.viewports[i] = D3D10_VIEWPORT { };
      m_state.rs.scissors [i] = D3D10_RECT     { };
    }
    
    // Default predication
    m_state.pr.predicateObject = nullptr;
    m_state.pr.predicateValue  = FALSE;
    
    // Make sure to apply all state
    RestoreState();
  }
  
  void STDMETHODCALLTYPE D3D10Device::SetPredication(
          ID3D10Predicate*                  pPredicate,
          BOOL                              PredicateValue) {
    static bool s_errorShown = false;
    
    if (!std::exchange(s_errorShown, true))
      Logger::err("D3D10Device::SetPredication: Stub");
    
    m_state.pr.predicateObject = static_cast<D3D10Query*>(pPredicate);
    m_state.pr.predicateValue  = PredicateValue;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::GetPredication(
          ID3D10Predicate**                 ppPredicate,
          BOOL*                             pPredicateValue) {
    if (ppPredicate != nullptr)
      *ppPredicate = m_state.pr.predicateObject.ref();
    
    if (pPredicateValue != nullptr)
      *pPredicateValue = m_state.pr.predicateValue;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::CopySubresourceRegion(
          ID3D10Resource*                   pDstResource,
          UINT                              DstSubresource,
          UINT                              DstX,
          UINT                              DstY,
          UINT                              DstZ,
          ID3D10Resource*                   pSrcResource,
          UINT                              SrcSubresource,
    const D3D10_BOX*                        pSrcBox) {
    D3D10_RESOURCE_DIMENSION dstResourceDim = D3D10_RESOURCE_DIMENSION_UNKNOWN;
    D3D10_RESOURCE_DIMENSION srcResourceDim = D3D10_RESOURCE_DIMENSION_UNKNOWN;
    
    pDstResource->GetType(&dstResourceDim);
    pSrcResource->GetType(&srcResourceDim);
    
    // Copying 2D image slices to 3D images and vice versa is legal
    const bool copy2Dto3D = dstResourceDim == D3D10_RESOURCE_DIMENSION_TEXTURE3D
                         && srcResourceDim == D3D10_RESOURCE_DIMENSION_TEXTURE2D;
    const bool copy3Dto2D = dstResourceDim == D3D10_RESOURCE_DIMENSION_TEXTURE2D
                         && srcResourceDim == D3D10_RESOURCE_DIMENSION_TEXTURE3D;
    
    if (dstResourceDim != srcResourceDim && !copy2Dto3D && !copy3Dto2D) {
      Logger::err(str::format(
        "D3D10: CopySubresourceRegion: Incompatible resources",
        "\n  Dst resource type: ", dstResourceDim,
        "\n  Src resource type: ", srcResourceDim));
      return;
    }
    
    if (dstResourceDim == D3D10_RESOURCE_DIMENSION_BUFFER) {
      auto dstBuffer = static_cast<D3D10Buffer*>(pDstResource)->GetBufferSlice();
      auto srcBuffer = static_cast<D3D10Buffer*>(pSrcResource)->GetBufferSlice();
      
      VkDeviceSize srcOffset = 0;
      VkDeviceSize srcLength = srcBuffer.length();
      
      if (pSrcBox != nullptr) {
        if (pSrcBox->left > pSrcBox->right)
          return;  // no-op, but legal
        
        srcOffset = pSrcBox->left;
        srcLength = pSrcBox->right - pSrcBox->left;
      }
      
      EmitCs([
        cDstSlice = dstBuffer.subSlice(DstX, srcLength),
        cSrcSlice = srcBuffer.subSlice(srcOffset, srcLength)
      ] (DxvkContext* ctx) {
        ctx->copyBuffer(
          cDstSlice.buffer(),
          cDstSlice.offset(),
          cSrcSlice.buffer(),
          cSrcSlice.offset(),
          cSrcSlice.length());
      });
    } else {
      const D3D10CommonTexture* dstTextureInfo = GetCommonTexture(pDstResource);
      const D3D10CommonTexture* srcTextureInfo = GetCommonTexture(pSrcResource);
      
      const Rc<DxvkImage> dstImage = dstTextureInfo->GetImage();
      const Rc<DxvkImage> srcImage = srcTextureInfo->GetImage();
      
      const DxvkFormatInfo* dstFormatInfo = imageFormatInfo(dstImage->info().format);
      const DxvkFormatInfo* srcFormatInfo = imageFormatInfo(srcImage->info().format);
      
      const VkImageSubresource dstSubresource = dstTextureInfo->GetSubresourceFromIndex(dstFormatInfo->aspectMask, DstSubresource);
      const VkImageSubresource srcSubresource = srcTextureInfo->GetSubresourceFromIndex(srcFormatInfo->aspectMask, SrcSubresource);
      
      VkOffset3D srcOffset = { 0, 0, 0 };
      VkOffset3D dstOffset = {
        static_cast<int32_t>(DstX),
        static_cast<int32_t>(DstY),
        static_cast<int32_t>(DstZ) };
      
      VkExtent3D extent = srcImage->mipLevelExtent(srcSubresource.mipLevel);
      
      if (pSrcBox != nullptr) {
        if (pSrcBox->left  >= pSrcBox->right
         || pSrcBox->top   >= pSrcBox->bottom
         || pSrcBox->front >= pSrcBox->back)
          return;  // no-op, but legal
        
        srcOffset.x = pSrcBox->left;
        srcOffset.y = pSrcBox->top;
        srcOffset.z = pSrcBox->front;
        
        extent.width  = pSrcBox->right -  pSrcBox->left;
        extent.height = pSrcBox->bottom - pSrcBox->top;
        extent.depth  = pSrcBox->back -   pSrcBox->front;
      }
      
      VkImageSubresourceLayers dstLayers = {
        dstSubresource.aspectMask,
        dstSubresource.mipLevel,
        dstSubresource.arrayLayer, 1 };
      
      VkImageSubresourceLayers srcLayers = {
        srcSubresource.aspectMask,
        srcSubresource.mipLevel,
        srcSubresource.arrayLayer, 1 };
      
      // Copying multiple slices does not
      // seem to be supported in D3D10
      if (copy2Dto3D || copy3Dto2D) {
        extent.depth         = 1;
        dstLayers.layerCount = 1;
        srcLayers.layerCount = 1;
      }
      
      EmitCs([
        cDstImage  = dstImage,
        cSrcImage  = srcImage,
        cDstLayers = dstLayers,
        cSrcLayers = srcLayers,
        cDstOffset = dstOffset,
        cSrcOffset = srcOffset,
        cExtent    = extent
      ] (DxvkContext* ctx) {
        ctx->copyImage(
          cDstImage, cDstLayers, cDstOffset,
          cSrcImage, cSrcLayers, cSrcOffset,
          cExtent);
      });
    }
  }
  
  void STDMETHODCALLTYPE D3D10Device::CopyResource(
          ID3D10Resource*                   pDstResource,
          ID3D10Resource*                   pSrcResource) {
    D3D10_RESOURCE_DIMENSION dstResourceDim = D3D10_RESOURCE_DIMENSION_UNKNOWN;
    D3D10_RESOURCE_DIMENSION srcResourceDim = D3D10_RESOURCE_DIMENSION_UNKNOWN;
    
    pDstResource->GetType(&dstResourceDim);
    pSrcResource->GetType(&srcResourceDim);
    
    if (dstResourceDim != srcResourceDim) {
      Logger::err(str::format(
        "D3D10: CopyResource: Incompatible resources",
        "\n  Dst resource type: ", dstResourceDim,
        "\n  Src resource type: ", srcResourceDim));
      return;
    }
    
    if (dstResourceDim == D3D10_RESOURCE_DIMENSION_BUFFER) {
      auto dstBuffer = static_cast<D3D10Buffer*>(pDstResource)->GetBufferSlice();
      auto srcBuffer = static_cast<D3D10Buffer*>(pSrcResource)->GetBufferSlice();
      
      if (dstBuffer.length() != srcBuffer.length()) {
        Logger::err(str::format(
          "D3D10: CopyResource: Mismatched buffer size",
          "\n  Dst buffer size: ", dstBuffer.length(),
          "\n  Src buffer size: ", srcBuffer.length()));
        return;
      }
      
      EmitCs([
        cDstBuffer = std::move(dstBuffer),
        cSrcBuffer = std::move(srcBuffer)
      ] (DxvkContext* ctx) {
        ctx->copyBuffer(
          cDstBuffer.buffer(),
          cDstBuffer.offset(),
          cSrcBuffer.buffer(),
          cSrcBuffer.offset(),
          cSrcBuffer.length());
      });
    } else {
      const Rc<DxvkImage> dstImage = GetCommonTexture(pDstResource)->GetImage();
      const Rc<DxvkImage> srcImage = GetCommonTexture(pSrcResource)->GetImage();

      const DxvkFormatInfo* dstFormatInfo = imageFormatInfo(dstImage->info().format);
      const DxvkFormatInfo* srcFormatInfo = imageFormatInfo(srcImage->info().format);
      
      for (uint32_t i = 0; i < srcImage->info().mipLevels; i++) {
        VkExtent3D extent = srcImage->mipLevelExtent(i);

        const VkImageSubresourceLayers dstLayers = { dstFormatInfo->aspectMask, i, 0, dstImage->info().numLayers };
        const VkImageSubresourceLayers srcLayers = { srcFormatInfo->aspectMask, i, 0, srcImage->info().numLayers };
        
        EmitCs([
          cDstImage  = dstImage,
          cSrcImage  = srcImage,
          cDstLayers = dstLayers,
          cSrcLayers = srcLayers,
          cExtent    = extent
        ] (DxvkContext* ctx) {
          ctx->copyImage(
            cDstImage, cDstLayers, VkOffset3D { 0, 0, 0 },
            cSrcImage, cSrcLayers, VkOffset3D { 0, 0, 0 },
            cExtent);
        });
      }
    }
  }
  
  void STDMETHODCALLTYPE D3D10Device::ClearRenderTargetView(
          ID3D10RenderTargetView*           pRenderTargetView,
    const FLOAT                             ColorRGBA[4]) {
    auto rtv = static_cast<D3D10RenderTargetView*>(pRenderTargetView);
    
    if (rtv == nullptr)
      return;
    
    const Rc<DxvkImageView> view = rtv->GetImageView();
    
    VkClearValue clearValue;
    clearValue.color.float32[0] = ColorRGBA[0];
    clearValue.color.float32[1] = ColorRGBA[1];
    clearValue.color.float32[2] = ColorRGBA[2];
    clearValue.color.float32[3] = ColorRGBA[3];
    
    VkClearRect clearRect;
    clearRect.rect.offset.x       = 0;
    clearRect.rect.offset.y       = 0;
    clearRect.rect.extent.width   = view->mipLevelExtent(0).width;
    clearRect.rect.extent.height  = view->mipLevelExtent(0).height;
    clearRect.baseArrayLayer      = 0;
    clearRect.layerCount          = view->info().numLayers;
    
    if (GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0)
      clearRect.layerCount        = 1;
    
    EmitCs([
      cClearValue = clearValue,
      cClearRect  = clearRect,
      cImageView  = view
    ] (DxvkContext* ctx) {
      ctx->clearRenderTarget(
        cImageView, cClearRect,
        VK_IMAGE_ASPECT_COLOR_BIT,
        cClearValue);
    });
  }
  
  void STDMETHODCALLTYPE D3D10Device::ClearDepthStencilView(
          ID3D10DepthStencilView*           pDepthStencilView,
          UINT                              ClearFlags,
          FLOAT                             Depth,
          UINT8                             Stencil) {
    auto dsv = static_cast<D3D10DepthStencilView*>(pDepthStencilView);
    
    if (dsv == nullptr)
      return;
    
    // Figure out which aspects to clear based
    // on the image format and the clear flags.
    const Rc<DxvkImageView> view = dsv->GetImageView();
    
    VkImageAspectFlags aspectMask = 0;
    
    if (ClearFlags & D3D10_CLEAR_DEPTH)
      aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    
    if (ClearFlags & D3D10_CLEAR_STENCIL)
      aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    
    aspectMask &= imageFormatInfo(view->info().format)->aspectMask;
    
    VkClearValue clearValue;
    clearValue.depthStencil.depth   = Depth;
    clearValue.depthStencil.stencil = Stencil;
    
    VkClearRect clearRect;
    clearRect.rect.offset.x       = 0;
    clearRect.rect.offset.y       = 0;
    clearRect.rect.extent.width   = view->mipLevelExtent(0).width;
    clearRect.rect.extent.height  = view->mipLevelExtent(0).height;
    clearRect.baseArrayLayer      = 0;
    clearRect.layerCount          = view->info().numLayers;
    
    if (GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0)
      clearRect.layerCount        = 1;
    
    EmitCs([
      cClearValue = clearValue,
      cClearRect  = clearRect,
      cAspectMask = aspectMask,
      cImageView  = view
    ] (DxvkContext* ctx) {
      ctx->clearRenderTarget(
        cImageView, cClearRect,
        cAspectMask, cClearValue);
    });
  }
  
  void STDMETHODCALLTYPE D3D10Device::GenerateMips(ID3D10ShaderResourceView* pShaderResourceView) {
    auto view = static_cast<D3D10ShaderResourceView*>(pShaderResourceView);
      
    if (view->GetResourceType() != D3D10_RESOURCE_DIMENSION_BUFFER) {
      EmitCs([cDstImageView = view->GetImageView()]
      (DxvkContext* ctx) {
        ctx->generateMipmaps(
          cDstImageView->image(),
          cDstImageView->subresources());
      });
    } else {
      Logger::err("D3D10: GenerateMips called on a buffer");
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::UpdateSubresource(
          ID3D10Resource*                   pDstResource,
          UINT                              DstSubresource,
    const D3D10_BOX*                        pDstBox,
    const void*                             pSrcData,
          UINT                              SrcRowPitch,
          UINT                              SrcDepthPitch) {
    // We need a different code path for buffers
    D3D10_RESOURCE_DIMENSION resourceType;
    pDstResource->GetType(&resourceType);
    
    if (resourceType == D3D10_RESOURCE_DIMENSION_BUFFER) {
      const auto bufferResource = static_cast<D3D10Buffer*>(pDstResource);
      const auto bufferSlice = bufferResource->GetBufferSlice();
      
      VkDeviceSize offset = bufferSlice.offset();
      VkDeviceSize size   = bufferSlice.length();
      
      if (pDstBox != nullptr) {
        offset = pDstBox->left;
        size   = pDstBox->right - pDstBox->left;
      }
      
      if (offset + size > bufferSlice.length()) {
        Logger::err(str::format(
          "D3D10: UpdateSubresource: Buffer update range out of bounds",
          "\n  Dst slice offset: ", bufferSlice.offset(),
          "\n  Dst slice length: ", bufferSlice.length(),
          "\n  Src slice offset: ", offset,
          "\n  Src slice length: ", size));
        return;
      }
      
      if (size == 0)
        return;
      
      if (((size == bufferSlice.length())
       && (bufferSlice.buffer()->memFlags() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))) {
		D3D10_RESOURCE_DIMENSION resourceDim = D3D10_RESOURCE_DIMENSION_UNKNOWN;
		pDstResource->GetType(&resourceDim);

		if (resourceDim == D3D10_RESOURCE_DIMENSION_BUFFER) {
			D3D10Buffer* pBuffer = static_cast<D3D10Buffer*>(pDstResource);
			void* pData = nullptr;
			pBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, &pData);

			if (pData)
				std::memcpy(pData, pSrcData, size);

			pBuffer->Unmap();
		}
		else if (resourceDim == D3D10_RESOURCE_DIMENSION_TEXTURE1D) {
			D3D10Texture1D* pTex = static_cast<D3D10Texture1D*>(pDstResource);
			void* pData = nullptr;
			pTex->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &pData);

			if (pData)
				std::memcpy(pData, pSrcData, size);

			pTex->Unmap(0);
		}
		else if (resourceDim == D3D10_RESOURCE_DIMENSION_TEXTURE2D) {
			D3D10Texture2D* pTex = static_cast<D3D10Texture2D*>(pDstResource);
			D3D10_MAPPED_TEXTURE2D data;
			pTex->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &data);

			if (data.pData)
				std::memcpy(data.pData, pSrcData, size);

			pTex->Unmap(0);
		}
		else if (resourceDim == D3D10_RESOURCE_DIMENSION_TEXTURE3D) {
			D3D10Texture3D* pTex = static_cast<D3D10Texture3D*>(pDstResource);
			D3D10_MAPPED_TEXTURE3D data;
			pTex->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &data);

			if (data.pData)
				std::memcpy(data.pData, pSrcData, size);

			pTex->Unmap(0);
		}
      } else {
        DxvkDataSlice dataSlice = AllocUpdateBufferSlice(size);
        std::memcpy(dataSlice.ptr(), pSrcData, size);
        
        EmitCs([
          cDataBuffer   = std::move(dataSlice),
          cBufferSlice  = bufferSlice.subSlice(offset, size)
        ] (DxvkContext* ctx) {
          ctx->updateBuffer(
            cBufferSlice.buffer(),
            cBufferSlice.offset(),
            cBufferSlice.length(),
            cDataBuffer.ptr());
        });
      }
    } else {
      const D3D10CommonTexture* textureInfo = GetCommonTexture(pDstResource);
      
      const VkImageSubresource subresource =
        textureInfo->GetSubresourceFromIndex(
          VK_IMAGE_ASPECT_COLOR_BIT, DstSubresource);
      
      VkOffset3D offset = { 0, 0, 0 };
      VkExtent3D extent = textureInfo->GetImage()->mipLevelExtent(subresource.mipLevel);
      
      if (pDstBox != nullptr) {
        if (pDstBox->left >= pDstBox->right
         || pDstBox->top >= pDstBox->bottom
         || pDstBox->front >= pDstBox->back)
          return;  // no-op, but legal
        
        offset.x = pDstBox->left;
        offset.y = pDstBox->top;
        offset.z = pDstBox->front;
        
        extent.width  = pDstBox->right - pDstBox->left;
        extent.height = pDstBox->bottom - pDstBox->top;
        extent.depth  = pDstBox->back - pDstBox->front;
      }
      
      const VkImageSubresourceLayers layers = {
        subresource.aspectMask,
        subresource.mipLevel,
        subresource.arrayLayer, 1 };
      
      auto formatInfo = imageFormatInfo(
        textureInfo->GetImage()->info().format);
      
      const VkExtent3D regionExtent = util::computeBlockCount(extent, formatInfo->blockSize);
      
      const VkDeviceSize bytesPerRow   = regionExtent.width  * formatInfo->elementSize;
      const VkDeviceSize bytesPerLayer = regionExtent.height * bytesPerRow;
      const VkDeviceSize bytesTotal    = regionExtent.depth  * bytesPerLayer;
      
      DxvkDataSlice imageDataBuffer = AllocUpdateBufferSlice(bytesTotal);
      
      util::packImageData(
        reinterpret_cast<char*>(imageDataBuffer.ptr()),
        reinterpret_cast<const char*>(pSrcData),
        regionExtent, formatInfo->elementSize,
        SrcRowPitch, SrcDepthPitch);
      
      EmitCs([
        cDstImage         = textureInfo->GetImage(),
        cDstLayers        = layers,
        cDstOffset        = offset,
        cDstExtent        = extent,
        cSrcData          = std::move(imageDataBuffer),
        cSrcBytesPerRow   = bytesPerRow,
        cSrcBytesPerLayer = bytesPerLayer
      ] (DxvkContext* ctx) {
        ctx->updateImage(cDstImage, cDstLayers,
          cDstOffset, cDstExtent, cSrcData.ptr(),
          cSrcBytesPerRow, cSrcBytesPerLayer);
      });
    }
  }  
  
  void STDMETHODCALLTYPE D3D10Device::ResolveSubresource(
          ID3D10Resource*                   pDstResource,
          UINT                              DstSubresource,
          ID3D10Resource*                   pSrcResource,
          UINT                              SrcSubresource,
          DXGI_FORMAT                       Format) {
    D3D10_RESOURCE_DIMENSION dstResourceType;
    D3D10_RESOURCE_DIMENSION srcResourceType;
    
    pDstResource->GetType(&dstResourceType);
    pSrcResource->GetType(&srcResourceType);
    
    if (dstResourceType != D3D10_RESOURCE_DIMENSION_TEXTURE2D
     || srcResourceType != D3D10_RESOURCE_DIMENSION_TEXTURE2D) {
      Logger::err(str::format(
        "D3D10: ResolveSubresource: Incompatible resources",
        "\n  Dst resource type: ", dstResourceType,
        "\n  Src resource type: ", srcResourceType));
      return;
    }
    
    auto dstTexture = static_cast<D3D10Texture2D*>(pDstResource);
    auto srcTexture = static_cast<D3D10Texture2D*>(pSrcResource);
    
    D3D10_TEXTURE2D_DESC dstDesc;
    D3D10_TEXTURE2D_DESC srcDesc;
    
    dstTexture->GetDesc(&dstDesc);
    srcTexture->GetDesc(&srcDesc);
    
    if (dstDesc.SampleDesc.Count != 1) {
      Logger::err(str::format(
        "D3D10: ResolveSubresource: Invalid sample counts",
        "\n  Dst sample count: ", dstDesc.SampleDesc.Count,
        "\n  Src sample count: ", srcDesc.SampleDesc.Count));
      return;
    }
    
    const D3D10CommonTexture* dstTextureInfo = GetCommonTexture(pDstResource);
    const D3D10CommonTexture* srcTextureInfo = GetCommonTexture(pSrcResource);
    
    const DXGI_VK_FORMAT_INFO dstFormatInfo = LookupFormat(dstDesc.Format, DXGI_VK_FORMAT_MODE_ANY);
    const DXGI_VK_FORMAT_INFO srcFormatInfo = LookupFormat(srcDesc.Format, DXGI_VK_FORMAT_MODE_ANY);
    
    const VkImageSubresource dstSubresource =
      dstTextureInfo->GetSubresourceFromIndex(
        dstFormatInfo.Aspect, DstSubresource);
    
    const VkImageSubresource srcSubresource =
      srcTextureInfo->GetSubresourceFromIndex(
        srcFormatInfo.Aspect, SrcSubresource);
    
    const VkImageSubresourceLayers dstSubresourceLayers = {
      dstSubresource.aspectMask,
      dstSubresource.mipLevel,
      dstSubresource.arrayLayer, 1 };
    
    const VkImageSubresourceLayers srcSubresourceLayers = {
      srcSubresource.aspectMask,
      srcSubresource.mipLevel,
      srcSubresource.arrayLayer, 1 };
    
    if (srcDesc.SampleDesc.Count == 1) {
      EmitCs([
        cDstImage  = dstTextureInfo->GetImage(),
        cSrcImage  = srcTextureInfo->GetImage(),
        cDstLayers = dstSubresourceLayers,
        cSrcLayers = srcSubresourceLayers
      ] (DxvkContext* ctx) {
        ctx->copyImage(
          cDstImage, cDstLayers, VkOffset3D { 0, 0, 0 },
          cSrcImage, cSrcLayers, VkOffset3D { 0, 0, 0 },
          cDstImage->mipLevelExtent(cDstLayers.mipLevel));
      });
    } else {
      const VkFormat format = LookupFormat(
        Format, DXGI_VK_FORMAT_MODE_ANY).Format;
      
      EmitCs([
        cDstImage  = dstTextureInfo->GetImage(),
        cSrcImage  = srcTextureInfo->GetImage(),
        cDstSubres = dstSubresourceLayers,
        cSrcSubres = srcSubresourceLayers,
        cFormat    = format
      ] (DxvkContext* ctx) {
        ctx->resolveImage(
          cDstImage, cDstSubres,
          cSrcImage, cSrcSubres,
          cFormat);
      });
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::DrawAuto() {
    Logger::err("D3D10Device::DrawAuto: Not implemented");
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::Draw(
          UINT            VertexCount,
          UINT            StartVertexLocation) {
    EmitCs([=] (DxvkContext* ctx) {
      ctx->draw(
        VertexCount, 1,
        StartVertexLocation, 0);
    });
    
    m_drawCount += 1;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::DrawIndexed(
          UINT            IndexCount,
          UINT            StartIndexLocation,
          INT             BaseVertexLocation) {
    EmitCs([=] (DxvkContext* ctx) {
      ctx->drawIndexed(
        IndexCount, 1,
        StartIndexLocation,
        BaseVertexLocation, 0);
    });
    
    m_drawCount += 1;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::DrawInstanced(
          UINT            VertexCountPerInstance,
          UINT            InstanceCount,
          UINT            StartVertexLocation,
          UINT            StartInstanceLocation) {
    EmitCs([=] (DxvkContext* ctx) {
      ctx->draw(
        VertexCountPerInstance,
        InstanceCount,
        StartVertexLocation,
        StartInstanceLocation);
    });
    
    m_drawCount += 1;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::DrawIndexedInstanced(
          UINT            IndexCountPerInstance,
          UINT            InstanceCount,
          UINT            StartIndexLocation,
          INT             BaseVertexLocation,
          UINT            StartInstanceLocation) {
    EmitCs([=] (DxvkContext* ctx) {
      ctx->drawIndexed(
        IndexCountPerInstance,
        InstanceCount,
        StartIndexLocation,
        BaseVertexLocation,
        StartInstanceLocation);
    });
    
    m_drawCount += 1;
  }
  
  void STDMETHODCALLTYPE D3D10Device::IASetInputLayout(ID3D10InputLayout* pInputLayout) {
    auto inputLayout = static_cast<D3D10InputLayout*>(pInputLayout);
    
    if (m_state.ia.inputLayout != inputLayout) {
      m_state.ia.inputLayout = inputLayout;
      ApplyInputLayout();
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY Topology) {
    if (m_state.ia.primitiveTopology != Topology) {
      m_state.ia.primitiveTopology = Topology;
      ApplyPrimitiveTopology();
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::IASetVertexBuffers(
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer* const*              ppVertexBuffers,
    const UINT*                             pStrides,
    const UINT*                             pOffsets) {
    for (uint32_t i = 0; i < NumBuffers; i++) {
      auto newBuffer = static_cast<D3D10Buffer*>(ppVertexBuffers[i]);
      
      m_state.ia.vertexBuffers[StartSlot + i].buffer = newBuffer;
      m_state.ia.vertexBuffers[StartSlot + i].offset = pOffsets[i];
      m_state.ia.vertexBuffers[StartSlot + i].stride = pStrides[i];
      
      BindVertexBuffer(StartSlot + i, newBuffer, pOffsets[i], pStrides[i]);
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::IASetIndexBuffer(
          ID3D10Buffer*                     pIndexBuffer,
          DXGI_FORMAT                       Format,
          UINT                              Offset) {
    auto newBuffer = static_cast<D3D10Buffer*>(pIndexBuffer);
    
    m_state.ia.indexBuffer.buffer = newBuffer;
    m_state.ia.indexBuffer.offset = Offset;
    m_state.ia.indexBuffer.format = Format;
    
    BindIndexBuffer(newBuffer, Offset, Format);
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::IAGetInputLayout(ID3D10InputLayout** ppInputLayout) {
    *ppInputLayout = m_state.ia.inputLayout.ref();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::IAGetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY* pTopology) {
    *pTopology = m_state.ia.primitiveTopology;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::IAGetVertexBuffers(
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer**                    ppVertexBuffers,
          UINT*                             pStrides,
          UINT*                             pOffsets) {
    for (uint32_t i = 0; i < NumBuffers; i++) {
      if (ppVertexBuffers != nullptr)
        ppVertexBuffers[i] = m_state.ia.vertexBuffers[StartSlot + i].buffer.ref();
      
      if (pStrides != nullptr)
        pStrides[i] = m_state.ia.vertexBuffers[StartSlot + i].stride;
      
      if (pOffsets != nullptr)
        pOffsets[i] = m_state.ia.vertexBuffers[StartSlot + i].offset;
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::IAGetIndexBuffer(
          ID3D10Buffer**                    ppIndexBuffer,
          DXGI_FORMAT*                      pFormat,
          UINT*                             pOffset) {
    if (ppIndexBuffer != nullptr)
      *ppIndexBuffer = m_state.ia.indexBuffer.buffer.ref();
    
    if (pFormat != nullptr)
      *pFormat = m_state.ia.indexBuffer.format;
    
    if (pOffset != nullptr)
      *pOffset = m_state.ia.indexBuffer.offset;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::VSSetShader(
          ID3D10VertexShader*               pVertexShader) {
    auto shader = static_cast<D3D10VertexShader*>(pVertexShader);
    
    if (m_state.vs.shader != shader) {
      m_state.vs.shader = shader;
      BindShader(shader, VK_SHADER_STAGE_VERTEX_BIT);
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::VSSetConstantBuffers(
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer* const*              ppConstantBuffers) {
    SetConstantBuffers(
      DxbcProgramType::VertexShader,
      m_state.vs.constantBuffers,
      StartSlot, NumBuffers,
      ppConstantBuffers,
      nullptr, nullptr);
  }


  void STDMETHODCALLTYPE D3D10Device::VSSetShaderResources(
          UINT                              StartSlot,
          UINT                              NumViews,
          ID3D10ShaderResourceView* const*  ppShaderResourceViews) {
    SetShaderResources(
      DxbcProgramType::VertexShader,
      m_state.vs.shaderResources,
      StartSlot, NumViews,
      ppShaderResourceViews);
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::VSSetSamplers(
          UINT                              StartSlot,
          UINT                              NumSamplers,
          ID3D10SamplerState* const*        ppSamplers) {
    SetSamplers(
      DxbcProgramType::VertexShader,
      m_state.vs.samplers,
      StartSlot, NumSamplers,
      ppSamplers);
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::VSGetShader(
          ID3D10VertexShader**              ppVertexShader) {
    if (ppVertexShader != nullptr)
      *ppVertexShader = m_state.vs.shader.ref();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::VSGetConstantBuffers(
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer**                    ppConstantBuffers) {
    GetConstantBuffers(
      m_state.vs.constantBuffers,
      StartSlot, NumBuffers,
      ppConstantBuffers,
      nullptr, nullptr);
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::VSGetShaderResources(
          UINT                              StartSlot,
          UINT                              NumViews,
          ID3D10ShaderResourceView**        ppShaderResourceViews) {
    for (uint32_t i = 0; i < NumViews; i++)
      ppShaderResourceViews[i] = m_state.vs.shaderResources.at(StartSlot + i).ref();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::VSGetSamplers(
          UINT                              StartSlot,
          UINT                              NumSamplers,
          ID3D10SamplerState**              ppSamplers) {
    for (uint32_t i = 0; i < NumSamplers; i++)
      ppSamplers[i] = m_state.vs.samplers.at(StartSlot + i).ref();
  }
  
  void STDMETHODCALLTYPE D3D10Device::GSSetShader(
          ID3D10GeometryShader*             pShader) {
    auto shader = static_cast<D3D10GeometryShader*>(pShader);
    
    if (m_state.gs.shader != shader) {
      m_state.gs.shader = shader;
      BindShader(shader, VK_SHADER_STAGE_GEOMETRY_BIT);
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::GSSetConstantBuffers(
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer* const*              ppConstantBuffers) {
    SetConstantBuffers(
      DxbcProgramType::GeometryShader,
      m_state.gs.constantBuffers,
      StartSlot, NumBuffers,
      ppConstantBuffers,
      nullptr, nullptr);
  }
  
  void STDMETHODCALLTYPE D3D10Device::GSSetShaderResources(
          UINT                              StartSlot,
          UINT                              NumViews,
          ID3D10ShaderResourceView* const*  ppShaderResourceViews) {
    SetShaderResources(
      DxbcProgramType::GeometryShader,
      m_state.gs.shaderResources,
      StartSlot, NumViews,
      ppShaderResourceViews);
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::GSSetSamplers(
          UINT                              StartSlot,
          UINT                              NumSamplers,
          ID3D10SamplerState* const*        ppSamplers) {
    SetSamplers(
      DxbcProgramType::GeometryShader,
      m_state.gs.samplers,
      StartSlot, NumSamplers,
      ppSamplers);
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::GSGetShader(
          ID3D10GeometryShader**            ppGeometryShader) {
    if (ppGeometryShader != nullptr)
      *ppGeometryShader = m_state.gs.shader.ref();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::GSGetConstantBuffers(
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer**                    ppConstantBuffers) {
    GetConstantBuffers(
      m_state.gs.constantBuffers,
      StartSlot, NumBuffers,
      ppConstantBuffers,
      nullptr, nullptr);
  }
  
  void STDMETHODCALLTYPE D3D10Device::GSGetShaderResources(
          UINT                              StartSlot,
          UINT                              NumViews,
          ID3D10ShaderResourceView**        ppShaderResourceViews) {
    for (uint32_t i = 0; i < NumViews; i++)
      ppShaderResourceViews[i] = m_state.gs.shaderResources.at(StartSlot + i).ref();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::GSGetSamplers(
          UINT                              StartSlot,
          UINT                              NumSamplers,
          ID3D10SamplerState**              ppSamplers) {
    for (uint32_t i = 0; i < NumSamplers; i++)
      ppSamplers[i] = m_state.gs.samplers.at(StartSlot + i).ref();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::PSSetShader(
          ID3D10PixelShader*                pPixelShader) {
    auto shader = static_cast<D3D10PixelShader*>(pPixelShader);
    
    if (m_state.ps.shader != shader) {
      m_state.ps.shader = shader;
      BindShader(shader, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::PSSetConstantBuffers(
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer* const*              ppConstantBuffers) {
    SetConstantBuffers(
      DxbcProgramType::PixelShader,
      m_state.ps.constantBuffers,
      StartSlot, NumBuffers,
      ppConstantBuffers,
      nullptr, nullptr);
  }
  
  void STDMETHODCALLTYPE D3D10Device::PSSetShaderResources(
          UINT                              StartSlot,
          UINT                              NumViews,
          ID3D10ShaderResourceView* const*  ppShaderResourceViews) {
    SetShaderResources(
      DxbcProgramType::PixelShader,
      m_state.ps.shaderResources,
      StartSlot, NumViews,
      ppShaderResourceViews);
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::PSSetSamplers(
          UINT                              StartSlot,
          UINT                              NumSamplers,
          ID3D10SamplerState* const*        ppSamplers) {
    SetSamplers(
      DxbcProgramType::PixelShader,
      m_state.ps.samplers,
      StartSlot, NumSamplers,
      ppSamplers);
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::PSGetShader(
          ID3D10PixelShader**               ppPixelShader) {
    if (ppPixelShader != nullptr)
      *ppPixelShader = m_state.ps.shader.ref();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::PSGetConstantBuffers(
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer**                    ppConstantBuffers) {
    GetConstantBuffers(
      m_state.ps.constantBuffers,
      StartSlot, NumBuffers,
      ppConstantBuffers,
      nullptr, nullptr);
  }
  
  void STDMETHODCALLTYPE D3D10Device::PSGetShaderResources(
          UINT                              StartSlot,
          UINT                              NumViews,
          ID3D10ShaderResourceView**        ppShaderResourceViews) {
    for (uint32_t i = 0; i < NumViews; i++)
      ppShaderResourceViews[i] = m_state.ps.shaderResources.at(StartSlot + i).ref();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::PSGetSamplers(
          UINT                              StartSlot,
          UINT                              NumSamplers,
          ID3D10SamplerState**              ppSamplers) {
    for (uint32_t i = 0; i < NumSamplers; i++)
      ppSamplers[i] = m_state.ps.samplers.at(StartSlot + i).ref();
  }  
  
  void STDMETHODCALLTYPE D3D10Device::OMSetRenderTargets(
          UINT                              NumViews,
          ID3D10RenderTargetView* const*    ppRenderTargetViews,
          ID3D10DepthStencilView*           pDepthStencilView) {
	// Optimization: If the number of draw and dispatch calls issued
    // prior to the previous context flush is above a certain threshold,
    // submit the current command buffer in order to keep the GPU busy.
    // This also helps keep the command buffers at a reasonable size.
    if (m_drawCount >= MaxPendingDraws)
      Flush();

    for (UINT i = 0; i < m_state.om.renderTargetViews.size(); i++) {
      D3D10RenderTargetView* view = nullptr;
      
      if ((i < NumViews) && (ppRenderTargetViews[i] != nullptr))
        view = static_cast<D3D10RenderTargetView*>(ppRenderTargetViews[i]);
      
      m_state.om.renderTargetViews.at(i) = view;
    }
    
    m_state.om.depthStencilView = static_cast<D3D10DepthStencilView*>(pDepthStencilView);
    
    BindFramebuffer();
  }
  
  void STDMETHODCALLTYPE D3D10Device::OMSetBlendState(
          ID3D10BlendState*                 pBlendState,
    const FLOAT                             BlendFactor[4],
          UINT                              SampleMask) {
    auto blendState = static_cast<D3D10BlendState*>(pBlendState);
    
    if (m_state.om.cbState    != blendState
     || m_state.om.sampleMask != SampleMask) {
      m_state.om.cbState    = blendState;
      m_state.om.sampleMask = SampleMask;
      
      ApplyBlendState();
    }
    
    if (BlendFactor != nullptr) {
      for (uint32_t i = 0; i < 4; i++)
        m_state.om.blendFactor[i] = BlendFactor[i];
      
      ApplyBlendFactor();
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::OMSetDepthStencilState(
          ID3D10DepthStencilState*          pDepthStencilState,
          UINT                              StencilRef) {
    auto depthStencilState = static_cast<D3D10DepthStencilState*>(pDepthStencilState);
    
    if (m_state.om.dsState != depthStencilState) {
      m_state.om.dsState = depthStencilState;
      ApplyDepthStencilState();
    }
    
    if (m_state.om.stencilRef != StencilRef) {
      m_state.om.stencilRef = StencilRef;
      ApplyStencilRef();
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::OMGetRenderTargets(
          UINT                              NumViews,
          ID3D10RenderTargetView**          ppRenderTargetViews,
          ID3D10DepthStencilView**          ppDepthStencilView) {
    if (ppRenderTargetViews != nullptr) {
      for (UINT i = 0; i < NumViews; i++)
        ppRenderTargetViews[i] = m_state.om.renderTargetViews[i].ref();
    }
    
    if (ppDepthStencilView != nullptr)
      *ppDepthStencilView = m_state.om.depthStencilView.ref();
  }
  
  void STDMETHODCALLTYPE D3D10Device::OMGetBlendState(
          ID3D10BlendState**                ppBlendState,
          FLOAT                             BlendFactor[4],
          UINT*                             pSampleMask) {
    if (ppBlendState != nullptr)
      *ppBlendState = m_state.om.cbState.ref();
    
    if (BlendFactor != nullptr)
      std::memcpy(BlendFactor, m_state.om.blendFactor, sizeof(FLOAT) * 4);
    
    if (pSampleMask != nullptr)
      *pSampleMask = m_state.om.sampleMask;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::OMGetDepthStencilState(
          ID3D10DepthStencilState**         ppDepthStencilState,
          UINT*                             pStencilRef) {
    if (ppDepthStencilState != nullptr)
      *ppDepthStencilState = m_state.om.dsState.ref();
    
    if (pStencilRef != nullptr)
      *pStencilRef = m_state.om.stencilRef;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::RSSetState(ID3D10RasterizerState* pRasterizerState) {
    auto rasterizerState = static_cast<D3D10RasterizerState*>(pRasterizerState);
    
    if (m_state.rs.state != rasterizerState) {
      m_state.rs.state = rasterizerState;
      
      // In D3D10, the rasterizer state defines whether the
      // scissor test is enabled, so we have to update the
      // scissor rectangles as well.
      ApplyRasterizerState();
      ApplyViewportState();
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::RSSetViewports(
          UINT                              NumViewports,
    const D3D10_VIEWPORT*                   pViewports) {
    m_state.rs.numViewports = NumViewports;
    
    for (uint32_t i = 0; i < NumViewports; i++)
      m_state.rs.viewports.at(i) = pViewports[i];
    
    ApplyViewportState();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::RSSetScissorRects(
          UINT                              NumRects,
    const D3D10_RECT*                       pRects) {
    m_state.rs.numScissors = NumRects;
    
    for (uint32_t i = 0; i < NumRects; i++)
      m_state.rs.scissors.at(i) = pRects[i];
    
    if (m_state.rs.state != nullptr) {
      D3D10_RASTERIZER_DESC rsDesc;
      m_state.rs.state->GetDesc(&rsDesc);
      
      if (rsDesc.ScissorEnable)
        ApplyViewportState();
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::RSGetState(ID3D10RasterizerState** ppRasterizerState) {
    if (ppRasterizerState != nullptr)
      *ppRasterizerState = m_state.rs.state.ref();
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::RSGetViewports(
          UINT*                             pNumViewports,
          D3D10_VIEWPORT*                   pViewports) {
    if (pViewports != nullptr) {
      for (uint32_t i = 0; i < *pNumViewports; i++) {
        if (i < m_state.rs.numViewports) {
          pViewports[i] = m_state.rs.viewports.at(i);
        } else {
          pViewports[i].TopLeftX = 0.0f;
          pViewports[i].TopLeftY = 0.0f;
          pViewports[i].Width    = 0.0f;
          pViewports[i].Height   = 0.0f;
          pViewports[i].MinDepth = 0.0f;
          pViewports[i].MaxDepth = 0.0f;
        }
      }
    }
    
    *pNumViewports = m_state.rs.numViewports;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::RSGetScissorRects(
          UINT*                             pNumRects,
          D3D10_RECT*                       pRects) {
    if (pRects != nullptr) {
      for (uint32_t i = 0; i < *pNumRects; i++) {
        if (i < m_state.rs.numScissors) {
          pRects[i] = m_state.rs.scissors.at(i);
        } else {
          pRects[i].left   = 0;
          pRects[i].top    = 0;
          pRects[i].right  = 0;
          pRects[i].bottom = 0;
        }
      }
    }
    
    *pNumRects = m_state.rs.numScissors;
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::SOSetTargets(
          UINT                              NumBuffers,
          ID3D10Buffer* const*              ppSOTargets,
    const UINT*                             pOffsets) {
    // TODO implement properly, including pOffsets
    for (uint32_t i = 0; i < 4; i++) {
      m_state.so.targets[i] = (ppSOTargets != nullptr && i < NumBuffers)
        ? static_cast<D3D10Buffer*>(ppSOTargets[i])
        : nullptr;
    }
  }
  
  
  void STDMETHODCALLTYPE D3D10Device::SOGetTargets(
          UINT                              NumBuffers,
          ID3D10Buffer**                    ppSOTargets,
		  UINT*								pOffsets) {
    for (uint32_t i = 0; i < NumBuffers; i++)
      ppSOTargets[i] = m_state.so.targets[i + pOffsets ? pOffsets[i] : 0].ref();
  }
  
  
  void D3D10Device::ApplyInputLayout() {
    if (m_state.ia.inputLayout != nullptr) {
      EmitCs([cInputLayout = m_state.ia.inputLayout] (DxvkContext* ctx) {
        cInputLayout->BindToContext(ctx);
      });
    } else {
      EmitCs([] (DxvkContext* ctx) {
        ctx->setInputLayout(0, nullptr, 0, nullptr);
      });
    }
  }
  
  
  void D3D10Device::ApplyPrimitiveTopology() {
    if (m_state.ia.primitiveTopology == D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED)
      return;
    
    const DxvkInputAssemblyState iaState =
      [Topology = m_state.ia.primitiveTopology] () -> DxvkInputAssemblyState {
        switch (Topology) {
        case D3D10_PRIMITIVE_TOPOLOGY_POINTLIST:
        return { VK_PRIMITIVE_TOPOLOGY_POINT_LIST, VK_FALSE, 0 };
          
        case D3D10_PRIMITIVE_TOPOLOGY_LINELIST:
        return { VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_FALSE, 0 };
          
        case D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP:
        return { VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_TRUE, 0 };
          
        case D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        return { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE, 0 };
            
        case D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        return { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_TRUE, 0 };
          
        case D3D10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
        return { VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY, VK_FALSE, 0 };
          
        case D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
        return { VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY, VK_TRUE, 0 };
          
        case D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
        return { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY, VK_FALSE, 0 };
          
        case D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
        return { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY, VK_TRUE, 0 };
          
        default:
        Logger::err(str::format("D3D10: Invalid primitive topology: ", Topology));
        return { };
      }
    }();
    
    EmitCs([iaState] (DxvkContext* ctx) {
      ctx->setInputAssemblyState(iaState);
    });
  }
  
  
  void D3D10Device::ApplyBlendState() {
    EmitCs([
      cBlendState = m_state.om.cbState != nullptr
        ? m_state.om.cbState
        : m_defaultBlendState,
      cSampleMask = m_state.om.sampleMask
    ] (DxvkContext* ctx) {
      cBlendState->BindToContext(ctx, cSampleMask);
    });
  }
  
  
  void D3D10Device::ApplyBlendFactor() {
    EmitCs([
      cBlendConstants = DxvkBlendConstants {
        m_state.om.blendFactor[0], m_state.om.blendFactor[1],
        m_state.om.blendFactor[2], m_state.om.blendFactor[3] }
    ] (DxvkContext* ctx) {
      ctx->setBlendConstants(cBlendConstants);
    });
  }
  
  
  void D3D10Device::ApplyDepthStencilState() {
    EmitCs([
      cDepthStencilState = m_state.om.dsState != nullptr
        ? m_state.om.dsState
        : m_defaultDepthStencilState
    ] (DxvkContext* ctx) {
      cDepthStencilState->BindToContext(ctx);
    });
  }
  
  
  void D3D10Device::ApplyStencilRef() {
    EmitCs([
      cStencilRef = m_state.om.stencilRef
    ] (DxvkContext* ctx) {
      ctx->setStencilReference(cStencilRef);
    });
  }
  
  
  void D3D10Device::ApplyRasterizerState() {
    EmitCs([
      cRasterizerState = m_state.rs.state != nullptr
        ? m_state.rs.state
        : m_defaultRasterizerState
    ] (DxvkContext* ctx) {
      cRasterizerState->BindToContext(ctx);
    });
  }
  
  
  void D3D10Device::ApplyViewportState() {
    // We cannot set less than one viewport in Vulkan, and
    // rendering with no active viewport is illegal anyway.
    if (m_state.rs.numViewports == 0)
      return;
    
    std::array<VkViewport, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> viewports;
    std::array<VkRect2D,   D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> scissors;
    
    // D3D10's coordinate system has its origin in the bottom left,
    // but the viewport coordinates are aligned to the top-left
    // corner so we can get away with flipping the viewport.
    for (uint32_t i = 0; i < m_state.rs.numViewports; i++) {
      const D3D10_VIEWPORT& vp = m_state.rs.viewports.at(i);
      
      viewports.at(i) = VkViewport {
        float(vp.TopLeftX), float(vp.Height + vp.TopLeftY),
		float(vp.Width),   -float(vp.Height),
		float(vp.MinDepth), float(vp.MaxDepth),
      };
    }
    
    // Scissor rectangles. Vulkan does not provide an easy way
    // to disable the scissor test, so we'll have to set scissor
    // rects that are at least as large as the framebuffer.
    bool enableScissorTest = false;
    
    if (m_state.rs.state != nullptr) {
      D3D10_RASTERIZER_DESC rsDesc;
      m_state.rs.state->GetDesc(&rsDesc);
      enableScissorTest = rsDesc.ScissorEnable;
    }
    
    for (uint32_t i = 0; i < m_state.rs.numViewports; i++) {
      // TODO D3D10 docs aren't clear about what should happen
      // when there are undefined scissor rects for a viewport.
      // Figure out what it does on Windows.
      if (enableScissorTest && (i < m_state.rs.numScissors)) {
        const D3D10_RECT& sr = m_state.rs.scissors.at(i);
        
        scissors.at(i) = VkRect2D {
          VkOffset2D { sr.left, sr.top },
          VkExtent2D {
            static_cast<uint32_t>(sr.right  - sr.left),
            static_cast<uint32_t>(sr.bottom - sr.top) } };
      } else {
        scissors.at(i) = VkRect2D {
          VkOffset2D { 0, 0 },
          VkExtent2D {
            D3D10_VIEWPORT_BOUNDS_MAX,
            D3D10_VIEWPORT_BOUNDS_MAX } };
      }
    }
    
    EmitCs([
      cViewportCount = m_state.rs.numViewports,
      cViewports     = viewports,
      cScissors      = scissors
    ] (DxvkContext* ctx) {
      ctx->setViewports(
        cViewportCount,
        cViewports.data(),
        cScissors.data());
    });
  }
  
  
  void D3D10Device::BindFramebuffer() {
    // NOTE According to the Microsoft docs, we are supposed to
    // unbind overlapping shader resource views. Since this comes
    // with a large performance penalty we'll ignore this until an
    // application actually relies on this behaviour.
    DxvkRenderTargets attachments;
    
    // D3D10 doesn't have the concept of a framebuffer object,
    // so we'll just create a new one every time the render
    // target bindings are updated. Set up the attachments.
    for (UINT i = 0; i < m_state.om.renderTargetViews.size(); i++) {
      if (m_state.om.renderTargetViews.at(i) != nullptr) {
        attachments.setColorTarget(i,
          m_state.om.renderTargetViews.at(i)->GetImageView(),
          m_state.om.renderTargetViews.at(i)->GetRenderLayout());
      }
    }
    
    if (m_state.om.depthStencilView != nullptr) {
      attachments.setDepthTarget(
        m_state.om.depthStencilView->GetImageView(),
        m_state.om.depthStencilView->GetRenderLayout());
    }
    
    // Create and bind the framebuffer object to the context
    EmitCs([cAttachments = std::move(attachments)] (DxvkContext* ctx) {
      ctx->bindRenderTargets(cAttachments);
    });
  }
  
  
  void D3D10Device::BindVertexBuffer(
          UINT                              Slot,
          D3D10Buffer*                      pBuffer,
          UINT                              Offset,
          UINT                              Stride) {
    EmitCs([
      cSlotId       = Slot,
      cBufferSlice  = pBuffer != nullptr ? pBuffer->GetBufferSlice(Offset) : DxvkBufferSlice(),
      cStride       = pBuffer != nullptr ? Stride                          : 0
    ] (DxvkContext* ctx) {
      ctx->bindVertexBuffer(cSlotId, cBufferSlice, cStride);
    });
  }
  
  
  void D3D10Device::BindIndexBuffer(
          D3D10Buffer*                      pBuffer,
          UINT                              Offset,
          DXGI_FORMAT                       Format) {
    // As in Vulkan, the index format can be either a 32-bit
    // or 16-bit unsigned integer, no other formats are allowed.
    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
    
    if (pBuffer != nullptr) {
      switch (Format) {
        case DXGI_FORMAT_R16_UINT: indexType = VK_INDEX_TYPE_UINT16; break;
        case DXGI_FORMAT_R32_UINT: indexType = VK_INDEX_TYPE_UINT32; break;
        default: Logger::err(str::format("D3D10: Invalid index format: ", Format));
      }
    }
    
    EmitCs([
      cBufferSlice  = pBuffer != nullptr ? pBuffer->GetBufferSlice(Offset) : DxvkBufferSlice(),
      cIndexType    = indexType
    ] (DxvkContext* ctx) {
      ctx->bindIndexBuffer(cBufferSlice, cIndexType);
    });
  }
  
  
  void D3D10Device::BindConstantBuffer(
          UINT                              Slot,
    const D3D10ConstantBufferBinding*       pBufferBinding) {
    EmitCs([
      cSlotId      = Slot,
      cBufferSlice = pBufferBinding->buffer != nullptr
        ? pBufferBinding->buffer->GetBufferSlice(
            pBufferBinding->constantOffset * 16,
            pBufferBinding->constantCount  * 16)
        : DxvkBufferSlice()
    ] (DxvkContext* ctx) {
      ctx->bindResourceBuffer(cSlotId, cBufferSlice);
    });
  }
  
  
  void D3D10Device::BindSampler(
          UINT                              Slot,
          D3D10SamplerState*                pSampler) {
    EmitCs([
      cSlotId   = Slot,
      cSampler  = pSampler != nullptr ? pSampler->GetDXVKSampler() : nullptr
    ] (DxvkContext* ctx) {
      ctx->bindResourceSampler(cSlotId, cSampler);
    });
  }
  
  
  void D3D10Device::BindShaderResource(
          UINT                              Slot,
          D3D10ShaderResourceView*          pResource) {
    EmitCs([
      cSlotId     = Slot,
      cImageView  = pResource != nullptr ? pResource->GetImageView()  : nullptr,
      cBufferView = pResource != nullptr ? pResource->GetBufferView() : nullptr
    ] (DxvkContext* ctx) {
      ctx->bindResourceView(cSlotId, cImageView, cBufferView);
    });
  }
  
  void D3D10Device::SetConstantBuffers(
          DxbcProgramType                   ShaderStage,
          D3D10ConstantBufferBindings&      Bindings,
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer* const*              ppConstantBuffers,
    const UINT*                             pFirstConstant,
    const UINT*                             pNumConstants) {
    const uint32_t slotId = computeResourceSlotId(
      ShaderStage, DxbcBindingType::ConstantBuffer,
      StartSlot);
    
    for (uint32_t i = 0; i < NumBuffers; i++) {
      auto newBuffer = static_cast<D3D10Buffer*>(ppConstantBuffers[i]);
      
      UINT constantOffset = 0;
      UINT constantCount  = newBuffer != nullptr
        ? newBuffer->GetSize() / 16
        : 0;
      
      if (newBuffer != nullptr && pFirstConstant != nullptr && pNumConstants != nullptr) {
        constantOffset = pFirstConstant[i];
        constantCount  = pNumConstants [i];
      }
      
      if (Bindings[StartSlot + i].buffer         != newBuffer
       || Bindings[StartSlot + i].constantOffset != constantOffset
       || Bindings[StartSlot + i].constantCount  != constantCount) {
        Bindings[StartSlot + i].buffer         = newBuffer;
        Bindings[StartSlot + i].constantOffset = constantOffset;
        Bindings[StartSlot + i].constantCount  = constantCount;
        
        BindConstantBuffer(slotId + i, &Bindings[StartSlot + i]);
      }
    }
  }
  
  
  void D3D10Device::SetSamplers(
          DxbcProgramType                   ShaderStage,
          D3D10SamplerBindings&             Bindings,
          UINT                              StartSlot,
          UINT                              NumSamplers,
          ID3D10SamplerState* const*        ppSamplers) {
    const uint32_t slotId = computeResourceSlotId(
      ShaderStage, DxbcBindingType::ImageSampler,
      StartSlot);
    
    for (uint32_t i = 0; i < NumSamplers; i++) {
      auto sampler = static_cast<D3D10SamplerState*>(ppSamplers[i]);
      
      if (Bindings[StartSlot + i] != sampler) {
        Bindings[StartSlot + i] = sampler;
        BindSampler(slotId + i, sampler);
      }
    }
  }
  
  
  void D3D10Device::SetShaderResources(
          DxbcProgramType                   ShaderStage,
          D3D10ShaderResourceBindings&      Bindings,
          UINT                              StartSlot,
          UINT                              NumResources,
          ID3D10ShaderResourceView* const*  ppResources) {
    const uint32_t slotId = computeResourceSlotId(
      ShaderStage, DxbcBindingType::ShaderResource,
      StartSlot);
    
    for (uint32_t i = 0; i < NumResources; i++) {
      auto resView = static_cast<D3D10ShaderResourceView*>(ppResources[i]);
      
      if (Bindings[StartSlot + i] != resView) {
        Bindings[StartSlot + i] = resView;
        BindShaderResource(slotId + i, resView);
      }
    }
  }
  
  
  void D3D10Device::GetConstantBuffers(
    const D3D10ConstantBufferBindings&      Bindings,
          UINT                              StartSlot,
          UINT                              NumBuffers,
          ID3D10Buffer**                    ppConstantBuffers, 
          UINT*                             pFirstConstant, 
          UINT*                             pNumConstants) {
    for (uint32_t i = 0; i < NumBuffers; i++) {
      if (ppConstantBuffers != nullptr)
        ppConstantBuffers[i] = Bindings[StartSlot + i].buffer.ref();
      
      if (pFirstConstant != nullptr)
        pFirstConstant[i] = Bindings[StartSlot + i].constantOffset;
      
      if (pNumConstants != nullptr)
        pNumConstants[i] = Bindings[StartSlot + i].constantCount;
    }
  }
  
  
  void D3D10Device::RestoreState() {
    BindFramebuffer();
    
    BindShader(m_state.vs.shader.ptr(), VK_SHADER_STAGE_VERTEX_BIT);
    BindShader(m_state.gs.shader.ptr(), VK_SHADER_STAGE_GEOMETRY_BIT);
    BindShader(m_state.ps.shader.ptr(), VK_SHADER_STAGE_FRAGMENT_BIT);
    
    ApplyInputLayout();
    ApplyPrimitiveTopology();
    ApplyBlendState();
    ApplyBlendFactor();
    ApplyDepthStencilState();
    ApplyStencilRef();
    ApplyRasterizerState();
    ApplyViewportState();
    
    BindIndexBuffer(
      m_state.ia.indexBuffer.buffer.ptr(),
      m_state.ia.indexBuffer.offset,
      m_state.ia.indexBuffer.format);
    
    for (uint32_t i = 0; i < m_state.ia.vertexBuffers.size(); i++) {
      BindVertexBuffer(i,
        m_state.ia.vertexBuffers[i].buffer.ptr(),
        m_state.ia.vertexBuffers[i].offset,
        m_state.ia.vertexBuffers[i].stride);
    }
    
    RestoreConstantBuffers(DxbcProgramType::VertexShader,   m_state.vs.constantBuffers);
    RestoreConstantBuffers(DxbcProgramType::GeometryShader, m_state.gs.constantBuffers);
    RestoreConstantBuffers(DxbcProgramType::PixelShader,    m_state.ps.constantBuffers);
    
    RestoreSamplers(DxbcProgramType::VertexShader,   m_state.vs.samplers);
    RestoreSamplers(DxbcProgramType::GeometryShader, m_state.gs.samplers);
    RestoreSamplers(DxbcProgramType::PixelShader,    m_state.ps.samplers);
    
    RestoreShaderResources(DxbcProgramType::VertexShader,   m_state.vs.shaderResources);
    RestoreShaderResources(DxbcProgramType::GeometryShader, m_state.gs.shaderResources);
    RestoreShaderResources(DxbcProgramType::PixelShader,    m_state.ps.shaderResources);
  }
  
  
  void D3D10Device::RestoreConstantBuffers(
          DxbcProgramType                   Stage,
          D3D10ConstantBufferBindings&      Bindings) {
    const uint32_t slotId = computeResourceSlotId(
      Stage, DxbcBindingType::ConstantBuffer, 0);
    
    for (uint32_t i = 0; i < Bindings.size(); i++)
      BindConstantBuffer(slotId + i, &Bindings[i]);
  }
  
  
  void D3D10Device::RestoreSamplers(
          DxbcProgramType                   Stage,
          D3D10SamplerBindings&             Bindings) {
    const uint32_t slotId = computeResourceSlotId(
      Stage, DxbcBindingType::ImageSampler, 0);
    
    for (uint32_t i = 0; i < Bindings.size(); i++)
      BindSampler(slotId + i, Bindings[i].ptr());
  }
  
  
  void D3D10Device::RestoreShaderResources(
          DxbcProgramType                   Stage,
          D3D10ShaderResourceBindings&      Bindings) {
    const uint32_t slotId = computeResourceSlotId(
      Stage, DxbcBindingType::ShaderResource, 0);
    
    for (uint32_t i = 0; i < Bindings.size(); i++)
      BindShaderResource(slotId + i, Bindings[i].ptr());
  }
  
  
  DxvkDataSlice D3D10Device::AllocUpdateBufferSlice(size_t Size) {
    constexpr size_t UpdateBufferSize = 4 * 1024 * 1024;
    
    if (Size >= UpdateBufferSize) {
      Rc<DxvkDataBuffer> buffer = new DxvkDataBuffer(Size);
      return buffer->alloc(Size);
    } else {
      if (m_updateBuffer == nullptr)
        m_updateBuffer = new DxvkDataBuffer(Size);
      
      DxvkDataSlice slice = m_updateBuffer->alloc(Size);
      
      if (slice.ptr() == nullptr) {
        m_updateBuffer = new DxvkDataBuffer(Size);
        slice = m_updateBuffer->alloc(Size);
      }
      
      return slice;
    }
  }

  // From Imm

  void STDMETHODCALLTYPE D3D10Device::Flush() {
	  FlushInitContext();

	  if (m_csIsBusy || m_csChunk->commandCount() != 0) {
		  // Add commands to flush the threaded
		  // context, then flush the command list
		  EmitCs([dev = m_dxvkDevice](DxvkContext* ctx) {
			  dev->submitCommandList(
				  ctx->endRecording(),
				  nullptr, nullptr);

			  ctx->beginRecording(
				  dev->createCommandList());
		  });

		  FlushCsChunk();

		  // Reset optimization info
		  m_drawCount = 0;
		  m_csIsBusy = false;
	  }
  }

  void D3D10Device::SetTextFilterSize(UINT, UINT)
  {
	  Logger::warn("D3D10Device::SetTextFilterSize: Stub");
  }

  void D3D10Device::GetTextFilterSize(UINT * w, UINT * h)
  {
	  Logger::warn("D3D10Device::GetTextFilterSize: Stub");
	  if (w)
		  *w = 0;

	  if (h)
		  *h = 0;
  }

  void D3D10Device::SynchronizeCsThread() {
	  // Dispatch current chunk so that all commands
	  // recorded prior to this function will be run
	  FlushCsChunk();

	  m_csThread->synchronize();
  }

  bool D3D10Device::WaitForResource(
	  const Rc<DxvkResource>&                 Resource,
	  UINT                              MapFlags) {
	  // Some games (e.g. The Witcher 3) do not work correctly
	  // when a map fails with D3D11_MAP_FLAG_DO_NOT_WAIT set
	  if (!TestOption(D3D10Option::AllowMapFlagNoWait))
		  MapFlags &= ~D3D10_MAP_FLAG_DO_NOT_WAIT;

	  // Wait for the any pending D3D11 command to be executed
	  // on the CS thread so that we can determine whether the
	  // resource is currently in use or not.
	  Flush();
	  SynchronizeCsThread();

	  if (Resource->isInUse()) {
		  if (MapFlags & D3D10_MAP_FLAG_DO_NOT_WAIT)
			  return false;

		  // TODO implement properly in DxvkDevice
		  while (Resource->isInUse())
			  std::this_thread::yield();
	  }

	  return true;
  }
  
  void D3D10Device::EmitCsChunk(Rc<DxvkCsChunk>&& chunk) {
	  m_csThread->dispatchChunk(std::move(chunk));
	  m_csIsBusy = true;
  }
}
