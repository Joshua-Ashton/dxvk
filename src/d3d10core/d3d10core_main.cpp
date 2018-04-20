#include <d3d10.h>
#include "../util/log/log.h"
#include "../dxgi/dxgi_include.h"

namespace dxvk {
	Logger Logger::s_instance("d3d10core.log");
}

extern "C" {
	using namespace dxvk;

	DLLEXPORT HRESULT __stdcall D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
		unsigned int flags, D3D_FEATURE_LEVEL feature_level, ID3D10Device **device)
	{
		Logger::warn("D3D10CoreCreateDevice: Stub");
		return E_NOTIMPL;
	}

	DLLEXPORT HRESULT __stdcall D3D10CoreRegisterLayers(void)
	{
		Logger::warn("D3D10CoreRegisterLayers: Stub");
		return E_NOTIMPL;
	}
}