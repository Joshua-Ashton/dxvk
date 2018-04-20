#pragma once

#include "../dxvk/dxvk_device.h"

#include "d3d10_1_device_child.h"

namespace dxvk {
  
  class D3D10Device;
  
  /**
   * \brief Render target view
   */
  class D3D10RenderTargetView : public D3D10DeviceChild<ID3D10RenderTargetView> {
    
  public:
    
    D3D10RenderTargetView(
            D3D10Device*                      device,
            ID3D10Resource*                   resource,
      const D3D10_RENDER_TARGET_VIEW_DESC&    desc,
      const Rc<DxvkImageView>&                view);
    
    ~D3D10RenderTargetView();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(ID3D10Device** ppDevice) final;
    
    void STDMETHODCALLTYPE GetResource(ID3D10Resource** ppResource) final;
    
    void STDMETHODCALLTYPE GetDesc(D3D10_RENDER_TARGET_VIEW_DESC* pDesc) final;
    
    D3D10_RESOURCE_DIMENSION GetResourceType() const {
      D3D10_RESOURCE_DIMENSION type;
      m_resource->GetType(&type);
      return type;
    }
    
    Rc<DxvkImageView> GetImageView() const {
      return m_view;
    }
    
    VkImageLayout GetRenderLayout() const {
      // Currently no reason to use anything else
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    
    static HRESULT GetDescFromResource(
            ID3D10Resource*                   pResource,
            D3D10_RENDER_TARGET_VIEW_DESC*    pDesc);
    
    static HRESULT NormalizeDesc(
            ID3D10Resource*                   pResource,
            D3D10_RENDER_TARGET_VIEW_DESC*    pDesc);
    
  private:
    
    Com<D3D10Device>                  m_device;
    Com<ID3D10Resource>               m_resource;
    D3D10_RENDER_TARGET_VIEW_DESC     m_desc;
    Rc<DxvkImageView>                 m_view;
    
  };
  
}
