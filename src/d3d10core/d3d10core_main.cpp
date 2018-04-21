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

	DLLEXPORT HRESULT __stdcall DXGID3D10CreateDevice(HMODULE d3d10core, IDXGIFactory *factory, IDXGIAdapter *adapter,
		unsigned int flags, const D3D_FEATURE_LEVEL *feature_levels, unsigned int level_count, void **device)
	{
		dxvk::Logger::warn("DXGID3D10CreateDevice: Stub");
		return E_NOTIMPL;
	}

	DLLEXPORT HRESULT __stdcall DXGID3D10RegisterLayers(const struct dxgi_device_layer *layers, UINT layer_count)
	{
		dxvk::Logger::warn("DXGID3D10RegisterLayers: Stub");
		return E_NOTIMPL;
	}

	DLLEXPORT void __stdcall D3D10CoreGetVersion()
	{
		dxvk::Logger::warn("D3D10CoreGetVersion: Stub");
	}
}