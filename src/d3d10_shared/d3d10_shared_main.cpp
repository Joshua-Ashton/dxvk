#include <d3d10_1.h>
#include "../dxgi/dxgi_adapter.h"
#include "../dxgi/dxgi_device.h"
#include "../replx/replx.h"

using namespace dxvk;
REPLXInterface* REPLXInterface::s_replxInterface = nullptr;

extern "C" {
	// Stubs...

	DLLEXPORT const char* __stdcall D3D10GetVertexShaderProfile(ID3D10Device *device)
	{
		Logger::warn("D3D10GetVertexShaderProfile: Stub");

		return "vs_4_0";
	}

	DLLEXPORT const char* __stdcall D3D10GetGeometryShaderProfile(ID3D10Device *device)
	{
		Logger::warn("D3D10GetGeometryShaderProfile: Stub");

		return "gs_4_0";
	}

	DLLEXPORT const char* __stdcall D3D10GetPixelShaderProfile(ID3D10Device *device)
	{
		Logger::warn("D3D10GetPixelShaderProfile: Stub");

		return "ps_4_0";
	}

	// Unknown params & returntype
	DLLEXPORT void __stdcall D3D10GetVersion()
	{
		Logger::warn("D3D10GetVersion: Stub");
	}

	DLLEXPORT void __stdcall D3D10RegisterLayers()
	{
		Logger::warn("D3D10RegisterLayers: Stub");
	}

	DLLEXPORT void __stdcall RevertToOldImplementation()
	{
		Logger::warn("RevertToOldImplementation: Stub");
	}
}