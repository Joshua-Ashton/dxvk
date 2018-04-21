#pragma once

#include <d3d10_1.h>
#include "../dxgi/dxgi_adapter.h"
#include "../dxgi/dxgi_device.h"

#define REPLX_ARGS(returntype, name, ...)

#define REPLX_WRAP(returntype, name, ...) typedef returntype ( __stdcall * REPLX##name )(__VA_ARGS__);
#include "replx_interfaces.h"
#undef REPLX_WRAP

class REPLXInterface
{
public:
	REPLXInterface()
	{
		HMODULE replxModule = LoadLibraryA("replx.dll");

		if (replxModule)
		{
#define REPLX_WRAP(returntype, name, ...) name = REPLX ## name (GetProcAddress(replxModule, "D3D10" #name ));
#include "replx_interfaces.h"
#undef REPLX_WRAP
		}
		else
		{
			DWORD error = GetLastError();
			dxvk::Logger::err(dxvk::str::format("Couldn't open replx, is it right arch? Error code: ", error));
		}
	}

	static REPLXInterface& Get()
	{
		if (!s_replxInterface)
			s_replxInterface = new REPLXInterface();

		return *s_replxInterface;
	}

#define REPLX_WRAP(returntype, name, ...) REPLX ## name name;
#include "replx_interfaces.h"
#undef REPLX_WRAP

private:
	static REPLXInterface* s_replxInterface;
};

extern "C" {
#undef REPLX_ARGS
#define REPLX_WRAP(returntype, name, ...) DLLEXPORT returntype __stdcall D3D10##name ( __VA_ARGS__ )
#define REPLX_ARGS(returntype, name, ...) { return REPLXInterface::Get().##name (__VA_ARGS__); }
#include "replx_interfaces.h"
}