#pragma once

#include <mutex>
#include <unordered_map>

#include "../dxbc/dxbc_module.h"
#include "../dxvk/dxvk_device.h"

#include "../util/sha1/sha1_util.h"

#include "../util/util_env.h"

#include "d3d10_1_device_child.h"
#include "d3d10_1_interfaces.h"

namespace dxvk {
  
  class D3D10Device;
  
  /**
   * \brief Shader key
   * 
   * A unique identifier for a shader consisting
   * of the program type and the SHA-1 hash of
   * the shader's original bytecode.
   */
  class D3D10ShaderKey {
    
  public:
    
    D3D10ShaderKey(
            DxbcProgramType ProgramType,
      const void*           pShaderBytecode,
            size_t          BytecodeLength);
    
    std::string GetName() const;
    
    size_t GetHash() const;
    
    bool operator == (const D3D10ShaderKey& other) const {
      return m_type == other.m_type
          && m_hash == other.m_hash;
    }
    
  private:
    
    DxbcProgramType m_type;
    Sha1Hash        m_hash;
    
  };
  
  struct D3D10ShaderKeyHash {
    size_t operator () (const D3D10ShaderKey& a) const {
      return a.GetHash();
    }
  };
  
  
  /**
   * \brief Shader module
   * 
   * Stores the compiled SPIR-V shader and the SHA-1
   * hash of the original DXBC shader, which can be
   * used to identify the shader.
   */
  class D3D10ShaderModule {
    
  public:
    
    D3D10ShaderModule();
    D3D10ShaderModule(
      const D3D10ShaderKey* pShaderKey,
      const DxbcOptions*    pDxbcOptions,
      const void*           pShaderBytecode,
            size_t          BytecodeLength);
    ~D3D10ShaderModule();
    
    Rc<DxvkShader> GetShader() const {
      return m_shader;
    }
    
    std::string GetName() const {
      return m_name;
    }
    
  private:
    
    std::string    m_name;
    Rc<DxvkShader> m_shader;
    
  };
  
  
  /**
   * \brief Common shader interface
   * 
   * Implements methods for all D3D10*Shader
   * interfaces and stores the actual shader
   * module object.
   */
  template<typename Base>
  class D3D10Shader : public D3D10DeviceChild<Base> {
    
  public:
    
    D3D10Shader(D3D10Device* device, const D3D10ShaderModule& module)
    : m_device(device), m_module(std::move(module)) { }
    
    ~D3D10Shader() { }
    
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) final {
      *ppvObject = nullptr;
      
      if (riid == __uuidof(IUnknown)
       || riid == __uuidof(ID3D10DeviceChild)
       || riid == __uuidof(Base)) {
        *ppvObject = ref(this);
        return S_OK;
      }
      
      Logger::warn("D3D10Shader::QueryInterface: Unknown interface query");
      return E_NOINTERFACE;
    }
    
    void STDMETHODCALLTYPE GetDevice(ID3D10Device **ppDevice) final {
      *ppDevice = m_device.ref();
    }
    
    Rc<DxvkShader> STDMETHODCALLTYPE GetShader() const {
      return m_module.GetShader();
    }
    
    const std::string& GetName() const {
      return m_module.GetName();
    }
    
  private:
    
    Com<D3D10Device>  m_device;
    D3D10ShaderModule m_module;
    
  };
  
  using D3D10VertexShader   = D3D10Shader<ID3D10VertexShader>;
  using D3D10GeometryShader = D3D10Shader<ID3D10GeometryShader>;
  using D3D10PixelShader    = D3D10Shader<ID3D10PixelShader>;
  
  
  /**
   * \brief Shader module set
   * 
   * Some applications may compile the same shader multiple
   * times, so we should cache the resulting shader modules
   * and reuse them rather than creating new ones. This
   * class is thread-safe.
   */
  class D3D10ShaderModuleSet {
    
  public:
    
    D3D10ShaderModuleSet();
    ~D3D10ShaderModuleSet();
    
    D3D10ShaderModule GetShaderModule(
      const DxbcOptions*    pDxbcOptions,
      const void*           pShaderBytecode,
            size_t          BytecodeLength,
            DxbcProgramType ProgramType);
    
  private:
    
    std::mutex m_mutex;
    
    std::unordered_map<
      D3D10ShaderKey,
      D3D10ShaderModule,
      D3D10ShaderKeyHash> m_modules;
    
  };
  
}
