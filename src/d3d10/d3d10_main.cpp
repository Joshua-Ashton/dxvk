#include <d3d10_1.h>

#include "../dxgi/dxgi_adapter.h"
#include "../dxgi/dxgi_device.h"

namespace dxvk {
	Logger Logger::s_instance("d3d10.log");
}

extern "C" {
	using namespace dxvk;

	DLLEXPORT HRESULT __stdcall D3D10CreateDevice
	(
		IDXGIAdapter      *pAdapter,
		D3D10_DRIVER_TYPE DriverType,
		HMODULE           Software,
		UINT              Flags,
		UINT              SDKVersion,
		ID3D10Device      **ppDevice
	)
	{
		return D3D10CreateDevice1(pAdapter, DriverType, Software, Flags, D3D10_FEATURE_LEVEL_10_0, SDKVersion, (ID3D10Device1**) ppDevice);
	}

	DLLEXPORT HRESULT __stdcall D3D10CreateDeviceAndSwapChain(
		IDXGIAdapter         *pAdapter,
		D3D10_DRIVER_TYPE    DriverType,
		HMODULE              Software,
		UINT                 Flags,
		UINT                 SDKVersion,
		DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
		IDXGISwapChain       **ppSwapChain,
		ID3D10Device         **ppDevice
	)
	{
		return D3D10CreateDeviceAndSwapChain1(pAdapter, DriverType, Software, Flags, D3D10_FEATURE_LEVEL_10_0, SDKVersion, pSwapChainDesc, ppSwapChain, (ID3D10Device1**)ppDevice);
	}
}