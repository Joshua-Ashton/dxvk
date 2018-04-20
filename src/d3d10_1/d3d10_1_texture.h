#pragma once

#include <dxvk_device.h>

#include "d3d10_1_device_child.h"
#include "d3d10_1_interfaces.h"

namespace dxvk {
  
  class D3D10Device;
  
  /**
   * \brief Image memory mapping mode
   * 
   * Determines how exactly \c Map will
   * behave when mapping an image.
   */
  enum D3D10_COMMON_TEXTURE_MAP_MODE {
    D3D10_COMMON_TEXTURE_MAP_MODE_NONE,   ///< Not mapped
    D3D10_COMMON_TEXTURE_MAP_MODE_BUFFER, ///< Mapped through buffer
    D3D10_COMMON_TEXTURE_MAP_MODE_DIRECT, ///< Directly mapped to host mem
  };
  
  
  /**
   * \brief Common texture description
   * 
   * Contains all members that can be
   * defined for 1D, 2D and 3D textures.
   */
  struct D3D10_COMMON_TEXTURE_DESC {
    UINT             Width;
    UINT             Height;
    UINT             Depth;
    UINT             MipLevels;
    UINT             ArraySize;
    DXGI_FORMAT      Format;
    DXGI_SAMPLE_DESC SampleDesc;
    D3D10_USAGE      Usage;
    UINT             BindFlags;
    UINT             CPUAccessFlags;
    UINT             MiscFlags;
  };
  
  
  /**
   * \brief D3D10 common texture object
   * 
   * This class implements common texture methods and
   * aims to work around the issue that there are three
   * different interfaces for basically the same thing.
   */
  class D3D10CommonTexture {
    
  public:
    
    D3D10CommonTexture(
            D3D10Device*                pDevice,
      const D3D10_COMMON_TEXTURE_DESC*  pDesc,
            D3D10_RESOURCE_DIMENSION    Dimension);
    
    ~D3D10CommonTexture();
    
    /**
     * \brief Texture properties
     * 
     * The returned data can be used to fill in
     * \c D3D10_TEXTURE2D_DESC and similar structs.
     * \returns Pointer to texture description
     */
    const D3D10_COMMON_TEXTURE_DESC* Desc() const {
      return &m_desc;
    }
    
    /**
     * \brief Map mode
     * \returns Map mode
     */
    D3D10_COMMON_TEXTURE_MAP_MODE GetMapMode() const {
      return m_mapMode;
    }
    
    /**
     * \brief The DXVK image
     * \returns The DXVK image
     */
    Rc<DxvkImage> GetImage() const {
      return m_image;
    }
    
    /**
     * \brief The DXVK buffer
     * \returns The DXVK buffer
     */
    Rc<DxvkBuffer> GetMappedBuffer() const {
      return m_buffer;
    }
    
    /**
     * \brief Currently mapped subresource
     * \returns Mapped subresource
     */
    VkImageSubresource GetMappedSubresource() const {
      return m_mappedSubresource;
    }
    
    /**
     * \brief Sets mapped subresource
     * \param [in] subresource THe subresource
     */
    void SetMappedSubresource(VkImageSubresource subresource) {
      m_mappedSubresource = subresource;
    }
    
    /**
     * \brief Resets mapped subresource
     * Marks the texture as not mapped.
     */
    void ClearMappedSubresource() {
      m_mappedSubresource = VkImageSubresource { };
    }
    
    /**
     * \brief Computes subresource from the subresource index
     * 
     * Used by some functions that operate on only
     * one subresource, such as \c UpdateSubresource.
     * \param [in] Aspect The image aspect
     * \param [in] Subresource Subresource index
     * \returns The Vulkan image subresource
     */
    VkImageSubresource GetSubresourceFromIndex(
            VkImageAspectFlags    Aspect,
            UINT                  Subresource) const;
    
    /**
     * \brief Format mode
     * 
     * Determines whether the image is going to
     * be used as a color image or a depth image.
     * \returns Format mode
     */
    DXGI_VK_FORMAT_MODE GetFormatMode() const;
    
    /**
     * \brief Retrieves parent D3D10 device
     * \param [out] ppDevice The device
     */
    void GetDevice(ID3D10Device** ppDevice) const;
    
    /**
     * \brief Normalizes and validates texture description
     * 
     * Fills in undefined values and validates the texture
     * parameters. Any error returned by this method should
     * be forwarded to the application.
     * \param [in,out] pDesc Texture description
     * \returns \c S_OK if the parameters are valid
     */
    static HRESULT NormalizeTextureProperties(
            D3D10_COMMON_TEXTURE_DESC* pDesc);

	void MapInternal(UINT Subresource,
		D3D10_MAP MapType,
		UINT MapFlags,
		void **ppData);
    
  private:
    
    Com<D3D10Device>              m_device;
    D3D10_COMMON_TEXTURE_DESC     m_desc;
    D3D10_COMMON_TEXTURE_MAP_MODE m_mapMode;
    
    Rc<DxvkImage>   m_image;
    Rc<DxvkBuffer>  m_buffer;
    
    VkImageSubresource m_mappedSubresource
      = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    
    Rc<DxvkBuffer> CreateMappedBuffer() const;
    
    BOOL CheckImageSupport(
      const DxvkImageCreateInfo*  pImageInfo,
            VkImageTiling         Tiling) const;
    
    D3D10_COMMON_TEXTURE_MAP_MODE DetermineMapMode(
      const DxvkImageCreateInfo*  pImageInfo) const;
    
    static VkImageType GetImageTypeFromResourceDim(
            D3D10_RESOURCE_DIMENSION  Dimension);
    
    static VkImageLayout OptimizeLayout(
            VkImageUsageFlags         Usage);
    
  };
  
  
  ///////////////////////////////////////////
  //      D 3 D 1 1 T E X T U R E 1 D
  class D3D10Texture1D : public D3D10DeviceChild<ID3D10Texture1D> {
    
  public:
    
    D3D10Texture1D(
            D3D10Device*                pDevice,
      const D3D10_COMMON_TEXTURE_DESC*  pDesc);
    
    ~D3D10Texture1D();
    
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
            D3D10_TEXTURE1D_DESC *pDesc) final;

	  HRESULT STDMETHODCALLTYPE Map(
		        UINT Subresource,
		        D3D10_MAP MapType,
		        UINT MapFlags,
		        void **ppData) final;

	  void STDMETHODCALLTYPE Unmap(
		        UINT Subresource) final;
    
    D3D10CommonTexture* GetCommonTexture() {
      return &m_texture;
    }
    
  private:
    
    D3D10CommonTexture m_texture;
    
  };
  
  
  ///////////////////////////////////////////
  //      D 3 D 1 1 T E X T U R E 2 D
  class D3D10Texture2D : public D3D10DeviceChild<ID3D10Texture2D> {
    
  public:
    
    D3D10Texture2D(
            D3D10Device*                pDevice,
      const D3D10_COMMON_TEXTURE_DESC*  pDesc);
    
    ~D3D10Texture2D();
    
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
            D3D10_TEXTURE2D_DESC *pDesc) final;
    
    D3D10CommonTexture* GetCommonTexture() {
      return &m_texture;
    }

    HRESULT STDMETHODCALLTYPE Map( 
              UINT Subresource,
              D3D10_MAP MapType,
              UINT MapFlags,
              D3D10_MAPPED_TEXTURE2D *pMappedTex2D) final;

    void STDMETHODCALLTYPE Unmap( 
              UINT Subresource) final;
    
  private:
    
    D3D10CommonTexture m_texture;
    
  };
  
  
  ///////////////////////////////////////////
  //      D 3 D 1 1 T E X T U R E 3 D
  class D3D10Texture3D : public D3D10DeviceChild<ID3D10Texture3D> {
    
  public:
    
    D3D10Texture3D(
            D3D10Device*                pDevice,
      const D3D10_COMMON_TEXTURE_DESC*  pDesc);
    
    ~D3D10Texture3D();
    
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
            D3D10_TEXTURE3D_DESC *pDesc) final;
    
    D3D10CommonTexture* GetCommonTexture() {
      return &m_texture;
    }

    HRESULT STDMETHODCALLTYPE Map( 
          UINT Subresource,
          D3D10_MAP MapType,
          UINT MapFlags,
          D3D10_MAPPED_TEXTURE3D *pMappedTex3D) final;

    void STDMETHODCALLTYPE Unmap( 
              UINT Subresource) final;
    
  private:
    
    D3D10CommonTexture m_texture;
    
  };
  
  
  /**
   * \brief Retrieves common info about a texture
   * 
   * \param [in] pResource The resource. Must be a texture.
   * \param [out] pTextureInfo Pointer to the texture info struct.
   * \returns E_INVALIDARG if the resource is not a texture
   */
  D3D10CommonTexture* GetCommonTexture(
          ID3D10Resource*       pResource);
  
}
