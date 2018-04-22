#include <d3d10_1.h>

#include "../dxgi/dxgi_adapter.h"
#include "../dxgi/dxgi_device.h"

namespace dxvk {
	Logger Logger::s_instance("d3d10.log");
}

extern "C" {
	using namespace dxvk;
    
    HRESULT WINAPI D3D10CreateDeviceAndSwapChain1(
    _In_opt_ IDXGIAdapter *pAdapter,
    D3D10_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    D3D10_FEATURE_LEVEL1 HardwareLevel,
    UINT SDKVersion,
    _In_opt_ DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    _Out_opt_ IDXGISwapChain **ppSwapChain,
    _Out_opt_ ID3D10Device1 **ppDevice);
    
    HRESULT WINAPI D3D10CreateDevice1(
    _In_opt_ IDXGIAdapter *pAdapter,
    D3D10_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    D3D10_FEATURE_LEVEL1 HardwareLevel,
    UINT SDKVersion,
    _Out_opt_ ID3D10Device1 **ppDevice);

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
