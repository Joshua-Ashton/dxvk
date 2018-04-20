#pragma once

#include "../dxgi/dxgi_device.h"
#include "../dxgi/dxgi_interfaces.h"

#include "d3d10_1_include.h"
#include "d3d10_1_texture.h"

namespace dxvk {
  
  class D3D10Device;
  
  class D3D10VkBackBuffer : public ComObject<IDXGIVkBackBuffer> {
    
  public:
    
    D3D10VkBackBuffer(D3D10Texture2D* pTexture)
    : m_texture(pTexture) { }
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID                  riid,
            void**                  ppvObject) final;
    
    Rc<DxvkImage> GetDXVKImage() final;
    
  public:
    
    Com<D3D10Texture2D> m_texture;
    
  };
  
  
  /**
   * \brief Present device
   * 
   * Wires up some swap chain related
   * functions to the D3D10 device.
   */
  class D3D10Presenter final : public IDXGIVkPresenter {
    
  public:
    
    D3D10Presenter(
            IDXGIObject*  pContainer,
            ID3D10Device* pDevice);
    ~D3D10Presenter();
    
    ULONG STDMETHODCALLTYPE AddRef();
    
    ULONG STDMETHODCALLTYPE Release();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID                  riid,
            void**                  ppvObject);
    
    HRESULT STDMETHODCALLTYPE CreateSwapChainBackBuffer(
      const DXGI_SWAP_CHAIN_DESC*   pSwapChainDesc,
            IDXGIVkBackBuffer**     ppInterface);
    
    HRESULT STDMETHODCALLTYPE FlushRenderingCommands();
    
    HRESULT STDMETHODCALLTYPE GetDevice(
            REFGUID                 riid,
            void**                  ppvDevice);
    
  private:
    
    IDXGIObject*  m_container;
    ID3D10Device* m_device;
    
  };
  
}
