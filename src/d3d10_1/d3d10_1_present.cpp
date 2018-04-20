#include "d3d10_1_device.h"
#include "d3d10_1_present.h"

namespace dxvk {
  
  HRESULT STDMETHODCALLTYPE D3D10VkBackBuffer::QueryInterface(REFIID riid, void** ppvObject) {
    return m_texture->QueryInterface(riid, ppvObject);
  }
  
  
  Rc<DxvkImage> D3D10VkBackBuffer::GetDXVKImage() {
    return m_texture->GetCommonTexture()->GetImage();
  }
  
  
  D3D10Presenter:: D3D10Presenter(
            IDXGIObject*  pContainer,
            ID3D10Device* pDevice)
  : m_container(pContainer), m_device(pDevice) {
    
  }
  
  
  D3D10Presenter::~D3D10Presenter() {
    
  }
  
  
  ULONG STDMETHODCALLTYPE D3D10Presenter::AddRef() {
    return m_container->AddRef();
  }
  
  
  ULONG STDMETHODCALLTYPE D3D10Presenter::Release() {
    return m_container->Release();
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Presenter::QueryInterface(REFIID riid, void** ppvObject) {
    return m_container->QueryInterface(riid, ppvObject);
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Presenter::CreateSwapChainBackBuffer(
    const DXGI_SWAP_CHAIN_DESC*       pSwapChainDesc,
          IDXGIVkBackBuffer**         ppInterface) {
    D3D10_COMMON_TEXTURE_DESC desc;
    desc.Width              = pSwapChainDesc->BufferDesc.Width;
    desc.Height             = pSwapChainDesc->BufferDesc.Height;
    desc.Depth              = 1;
    desc.MipLevels          = 1;
    desc.ArraySize          = 1;
    desc.Format             = pSwapChainDesc->BufferDesc.Format;
    desc.SampleDesc         = pSwapChainDesc->SampleDesc;
    desc.Usage              = D3D10_USAGE_DEFAULT;
    desc.BindFlags          = D3D10_BIND_RENDER_TARGET
                            | D3D10_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags     = 0;
    desc.MiscFlags          = 0;
    
    try {
      *ppInterface = ref(new D3D10VkBackBuffer(
        new D3D10Texture2D(static_cast<D3D10Device*>(m_device), &desc)));
      return S_OK;
    } catch (const DxvkError& e) {
      Logger::err(e.message());
      return E_FAIL;
    }
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Presenter::FlushRenderingCommands() {
    // The presentation code is run from the main rendering thread
    // rather than the command stream thread, so we synchronize.
    auto device = static_cast<D3D10Device*>(m_device);
	device->Flush();
	device->SynchronizeCsThread();
    return S_OK;
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D10Presenter::GetDevice(REFGUID riid, void** ppvDevice) {
    return m_device->QueryInterface(riid, ppvDevice);
  }
  
}
