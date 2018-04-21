#include <d3d10_1.h>

#include "../dxgi/dxgi_adapter.h"
#include "../dxgi/dxgi_device.h"

#include "d3d10_1_device.h"
#include "d3d10_1_enums.h"
#include "d3d10_1_present.h"

namespace dxvk {
	Logger Logger::s_instance("d3d10_1.log");
}

extern "C" {
	using namespace dxvk;

	DLLEXPORT HRESULT __stdcall D3D10CreateDevice1
	(
		IDXGIAdapter      *pAdapter,
		D3D10_DRIVER_TYPE DriverType,
		HMODULE           Software,
		UINT              Flags,
		D3D10_FEATURE_LEVEL1 HardwareLevel,
		UINT              SDKVersion,
		ID3D10Device1      **ppDevice
	)
	{
		Com<IDXGIAdapter>        dxgiAdapter = pAdapter;
		Com<IDXGIVkAdapter> dxvkAdapter = nullptr;

		if (dxgiAdapter == nullptr) {
			// We'll treat everything as hardware, even if the
			// Vulkan device is actually a software device.
			if (DriverType != D3D10_DRIVER_TYPE_HARDWARE)
				Logger::warn("D3D10CreateDevice1: Unsupported driver type");

			// We'll use the first adapter returned by a DXGI factory
			Com<IDXGIFactory> factory = nullptr;

			if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory)))) {
				Logger::err("D3D10CreateDevice1: Failed to create a DXGI factory");
				return E_FAIL;
			}

			if (FAILED(factory->EnumAdapters(0, &dxgiAdapter))) {
				Logger::err("D3D10CreateDevice1: No default adapter available");
				return E_FAIL;
			}

		}

		// The adapter must obviously be a DXVK-compatible adapter so
		// that we can create a DXVK-compatible DXGI device from it.
		if (FAILED(dxgiAdapter->QueryInterface(__uuidof(IDXGIVkAdapter),
			reinterpret_cast<void**>(&dxvkAdapter)))) {
			Logger::err("D3D10CreateDevice1: Adapter is not a DXVK adapter");
			return E_INVALIDARG;
		}

		// Feature levels to probe if the
		// application does not specify any.
		UINT FeatureLevels;
		std::array<D3D10_FEATURE_LEVEL1, 5> defaultFeatureLevels = {
			D3D10_FEATURE_LEVEL_10_1,
			D3D10_FEATURE_LEVEL_10_0, D3D10_FEATURE_LEVEL_9_3,
			D3D10_FEATURE_LEVEL_9_2,  D3D10_FEATURE_LEVEL_9_1,
		};

		
		D3D10_FEATURE_LEVEL1* pFeatureLevels = defaultFeatureLevels.data();
		FeatureLevels = defaultFeatureLevels.size();

		if (HardwareLevel == 0)
			HardwareLevel = D3D10_FEATURE_LEVEL_10_0;

		// Find the highest feature level supported by the device.
		// This works because the feature level array is ordered.
		const Rc<DxvkAdapter> adapter = dxvkAdapter->GetDXVKAdapter();

		UINT flId;
		for (flId = 0; flId < FeatureLevels; flId++) {
			Logger::info(str::format("D3D10CreateDevice1: Probing ", pFeatureLevels[flId]));

			if (D3D10Device::CheckFeatureLevelSupport(adapter, pFeatureLevels[flId]))
				break;
		}

		if (flId == FeatureLevels) {
			Logger::err("D3D10CreateDevice1: Requested feature level not supported");
			return E_INVALIDARG;
		}

		// Try to create the device with the given parameters.
		const D3D10_FEATURE_LEVEL1 fl = pFeatureLevels[flId];

		try {
			Logger::info(str::format("D3D10CreateDevice1: Using feature level ", fl));

			// If we cannot write back the device, don't make it at all.
			if (ppDevice == nullptr)
				return S_FALSE;

			Com<D3D10DeviceContainer> container = new D3D10DeviceContainer();

			const VkPhysicalDeviceFeatures deviceFeatures
				= D3D10Device::GetDeviceFeatures(adapter, fl);

			if (FAILED(dxvkAdapter->CreateDevice(container.ptr(), &deviceFeatures, &container->m_dxgiDevice))) {
				Logger::err("D3D10CreateDevice1: Failed to create DXGI device");
				return E_FAIL;
			}

			container->m_d3d10Device = new D3D10Device(
				container.ptr(), container->m_dxgiDevice, fl, Flags);

			container->m_d3d10Presenter = new D3D10Presenter(
				container.ptr(), container->m_d3d10Device);

			if (ppDevice != nullptr)
				*ppDevice = ref(container->m_d3d10Device);
			return S_OK;
		}
		catch (const DxvkError& e) {
			Logger::err("D3D10CreateDevice1: Failed to create D3D10 device");
			return E_FAIL;
		}
	}

	DLLEXPORT HRESULT __stdcall D3D10CreateDeviceAndSwapChain1(
		IDXGIAdapter         *pAdapter,
		D3D10_DRIVER_TYPE    DriverType,
		HMODULE              Software,
		UINT                 Flags,
		D3D10_FEATURE_LEVEL1 HardwareLevel,
		UINT                 SDKVersion,
		DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
		IDXGISwapChain       **ppSwapChain,
		ID3D10Device1         **ppDevice
	)
	{
		Com<ID3D10Device1>        d3d10Device;

		// Try to create a device first.
		HRESULT status = D3D10CreateDevice1(pAdapter, DriverType,
			Software, Flags, HardwareLevel,
			SDKVersion, &d3d10Device);

		if (FAILED(status))
			return status;

		// Again, the documentation does not exactly tell us what we
		// need to do in case one of the arguments is a null pointer.
		if (pSwapChainDesc == nullptr)
			return E_INVALIDARG;

		Com<IDXGIDevice>  dxgiDevice = nullptr;
		Com<IDXGIAdapter> dxgiAdapter = nullptr;
		Com<IDXGIFactory> dxgiFactory = nullptr;

		if (FAILED(d3d10Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice)))) {
			Logger::err("D3D10CreateDeviceAndSwapChain: Failed to query DXGI device");
			return E_FAIL;
		}

		if (FAILED(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapter)))) {
			Logger::err("D3D10CreateDeviceAndSwapChain: Failed to query DXGI adapter");
			return E_FAIL;
		}

		if (FAILED(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgiFactory)))) {
			Logger::err("D3D10CreateDeviceAndSwapChain: Failed to query DXGI factory");
			return E_FAIL;
		}

		DXGI_SWAP_CHAIN_DESC desc = *pSwapChainDesc;
		if (FAILED(dxgiFactory->CreateSwapChain(d3d10Device.ptr(), &desc, ppSwapChain))) {
			Logger::err("D3D11CreateDeviceAndSwapChain: Failed to create swap chain");
			return E_FAIL;
		}

		if (ppDevice != nullptr)
			*ppDevice = d3d10Device.ref();

		return S_OK;
	}
}