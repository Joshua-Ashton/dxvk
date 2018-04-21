#include <d3d10_1.h>
#include "../dxgi/dxgi_adapter.h"
#include "../dxgi/dxgi_device.h"

using namespace dxvk;

typedef HRESULT (__stdcall *REPLXCreateEffectFromMemory)(void *data, SIZE_T data_size, UINT flags,
	ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3D10Effect **effect);

typedef HRESULT (__stdcall *REPLXCompileEffectFromMemory)(void *data, SIZE_T data_size, const char *filename,
	const D3D10_SHADER_MACRO *defines, ID3D10Include *include, UINT hlsl_flags, UINT fx_flags,
	ID3D10Blob **effect, ID3D10Blob **errors);

class REPLXInterface
{
public:
	REPLXInterface()
	{
		HMODULE replxModule = LoadLibraryA("replx.dll");

		if (replxModule)
		{
			CreateEffect = REPLXCreateEffectFromMemory(GetProcAddress(replxModule, "D3D10CreateEffectFromMemory"));
			CompileEffect = REPLXCompileEffectFromMemory(GetProcAddress(replxModule, "D3D10CompileEffectFromMemory"));
		}
		else
		{
			DWORD error = GetLastError();
			Logger::err(str::format("Couldn't open replx: ", error));
		}
	}

	static REPLXInterface& Get()
	{
		if (!s_replxInterface)
			s_replxInterface = new REPLXInterface();

		return *s_replxInterface;
	}

	REPLXCreateEffectFromMemory CreateEffect;
	REPLXCompileEffectFromMemory CompileEffect;

private:
	static REPLXInterface* s_replxInterface;
};

REPLXInterface* REPLXInterface::s_replxInterface = nullptr;

extern "C" {
	DLLEXPORT HRESULT __stdcall D3D10CreateEffectFromMemory(void *data, SIZE_T data_size, UINT flags,
		ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3D10Effect **effect)
	{
		return REPLXInterface::Get().CreateEffect(data, data_size, flags, device, effect_pool, effect);
	}

	// Stubs...
	DLLEXPORT HRESULT __stdcall D3D10CompileEffectFromMemory(void *data, SIZE_T data_size, const char *filename,
		const D3D10_SHADER_MACRO *defines, ID3D10Include *include, UINT hlsl_flags, UINT fx_flags,
		ID3D10Blob **effect, ID3D10Blob **errors)
	{
		Logger::warn("D3D10CompileEffectFromMemory: Stub");

		if (effect)
			*effect = nullptr;

		if (errors)
			*errors = nullptr;

		return REPLXInterface::Get().CompileEffect(data, data_size, filename, defines, include, hlsl_flags, fx_flags, effect, errors);
	}

	DLLEXPORT HRESULT __stdcall D3D10DisassembleEffect(ID3D10Effect *pEffect, BOOL EnableColorCode, ID3D10Blob** ppDisassembly)
	{
		Logger::warn("D3D10DisassembleEffect: Stub");

		return E_NOTIMPL;
	}

	DLLEXPORT HRESULT __stdcall D3D10PreprocessShader(
		       LPCSTR             pSrcData,
		       SIZE_T             SrcDataSize,
		       LPCSTR             pFileName,
		 const D3D10_SHADER_MACRO *pDefines,
		       LPD3D10INCLUDE     pInclude,
		       ID3D10Blob         **ppShaderText,
		       ID3D10Blob         **ppErrorMsgs)
	
	{
		Logger::warn("D3D10PreprocessShader: Stub");

		if (ppShaderText)
			*ppShaderText = nullptr;

		if (ppErrorMsgs)
			*ppErrorMsgs = nullptr;

		return E_NOTIMPL;
	}

	DLLEXPORT HRESULT __stdcall D3D10CreateEffectPoolFromMemory(void *data, SIZE_T data_size, UINT fx_flags,
		ID3D10Device *device, ID3D10EffectPool **effect_pool)
	{
		Logger::warn("D3D10CreateEffectPoolFromMemory: Stub");

		return E_NOTIMPL;
	}

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
		Logger::warn("D3D10RegisterLayers: Stub");
	}
}