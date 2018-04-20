#pragma once

#include "../dxvk/dxvk_device.h"

#include "d3d10_1_device_child.h"

namespace dxvk {
  
  class D3D10Device;
  
  /**
   * \brief Shader resource view
   */
  class D3D10ShaderResourceView : public D3D10DeviceChild<ID3D10ShaderResourceView1> {
    
  public:
    
    D3D10ShaderResourceView(
            D3D10Device*                      device,
            ID3D10Resource*                   resource,
      const D3D10_SHADER_RESOURCE_VIEW_DESC1&  desc,
      const Rc<DxvkBufferView>&               bufferView);
    
    D3D10ShaderResourceView(
            D3D10Device*                      device,
            ID3D10Resource*                   resource,
      const D3D10_SHADER_RESOURCE_VIEW_DESC1&  desc,
      const Rc<DxvkImageView>&                imageView);
    
    ~D3D10ShaderResourceView();
    
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) final;
    
    void STDMETHODCALLTYPE GetDevice(ID3D10Device** ppDevice) final;
    
    void STDMETHODCALLTYPE GetResource(ID3D10Resource** ppResource) final;
    
    void STDMETHODCALLTYPE GetDesc(D3D10_SHADER_RESOURCE_VIEW_DESC* pDesc) final;
    void STDMETHODCALLTYPE GetDesc1(D3D10_SHADER_RESOURCE_VIEW_DESC1* pDesc) final;
    
    D3D10_RESOURCE_DIMENSION GetResourceType() const {
      D3D10_RESOURCE_DIMENSION type;
      m_resource->GetType(&type);
      return type;
    }
    
    Rc<DxvkBufferView> GetBufferView() const {
      return m_bufferView;
    }
    
    Rc<DxvkImageView> GetImageView() const {
      return m_imageView;
    }
    
    static HRESULT GetDescFromResource(
            ID3D10Resource*                   pResource,
            D3D10_SHADER_RESOURCE_VIEW_DESC1*  pDesc);
    
    static HRESULT NormalizeDesc(
            ID3D10Resource*                   pResource,
            D3D10_SHADER_RESOURCE_VIEW_DESC1*  pDesc);
    
  private:
    
    Com<D3D10Device>                  m_device;
    Com<ID3D10Resource>               m_resource;
    D3D10_SHADER_RESOURCE_VIEW_DESC1  m_desc;
    Rc<DxvkBufferView>                m_bufferView;
    Rc<DxvkImageView>                 m_imageView;
    
  };
  
}
