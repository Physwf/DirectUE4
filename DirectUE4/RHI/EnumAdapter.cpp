#include <dxgi.h>
#include <d3d11.h>
#include <stdio.h>
#include "log.h"


void EnumAdapters()
{
	IDXGIFactory* DXGIFactory = NULL;

	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&DXGIFactory);
	if (FAILED(hr))
	{
		X_LOG("CreateDXGIFactory failed!");
		return;
	}

	IDXGIAdapter* DXGIAdapter = NULL;
	for (UINT AdapterIndex = 0; SUCCEEDED(DXGIFactory->EnumAdapters(AdapterIndex, &DXGIAdapter)); ++AdapterIndex)
	{
		if (DXGIAdapter)
		{
			DXGI_ADAPTER_DESC DESC;
			if (SUCCEEDED(DXGIAdapter->GetDesc(&DESC)))
			{
				X_LOG("Adapter %d: Description:%ls  VendorId:%x, DeviceId:0x%x, SubSysId:0x%x Revision:%u DedicatedVideoMemory:%uM DedicatedSystemMemory:%uM SharedSystemMemory:%uM \n",
					AdapterIndex,
					DESC.Description,
					DESC.VendorId,
					DESC.DeviceId,
					DESC.SubSysId,
					DESC.Revision,
					DESC.DedicatedVideoMemory / (1024*1024),
					DESC.DedicatedSystemMemory / (1024 * 1024),
					DESC.SharedSystemMemory / (1024 * 1024)
					/*DESC.AdapterLuid*/);
			}
			IDXGIOutput* DXGIOutput = NULL;
			for (UINT OutputIndex = 0; SUCCEEDED(DXGIAdapter->EnumOutputs(OutputIndex, &DXGIOutput));++OutputIndex)
			{
				if (DXGIOutput)
				{
					DXGI_OUTPUT_DESC OutDesc;
					if (SUCCEEDED(DXGIOutput->GetDesc(&OutDesc)))
					{
						X_LOG("\tOutput:%u, DeviceName:%ls, Width:%u Height:%u \n", OutputIndex, OutDesc.DeviceName, OutDesc.DesktopCoordinates.right - OutDesc.DesktopCoordinates.left, OutDesc.DesktopCoordinates.bottom - OutDesc.DesktopCoordinates.top);
					}
				}
			}
		}

		
	}

	DXGIFactory->Release();
}

void EnumAdapters1()
{
	IDXGIFactory1* DXGIFactor1 = NULL;

	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&DXGIFactor1);
	if (FAILED(hr))
	{
		X_LOG("CreateDXGIFactory1 failed!");
		return;
	}
	IDXGIAdapter1* DXGIAdapter1 = NULL;
	for (UINT AdapterIndex = 0; SUCCEEDED(DXGIFactor1->EnumAdapters1(AdapterIndex, &DXGIAdapter1)); ++AdapterIndex)
	{
		if (DXGIAdapter1)
		{
			DXGI_ADAPTER_DESC1 AdapterDesc1;
			if (SUCCEEDED(DXGIAdapter1->GetDesc1(&AdapterDesc1)))
			{
				X_LOG("Adapter %d: Description:%ls  VendorId:%x, DeviceId:0x%x, SubSysId:0x%x Revision:%u DedicatedVideoMemory:%uM DedicatedSystemMemory:%uM SharedSystemMemory:%uM  Flags:%u\n",
					AdapterIndex,
					AdapterDesc1.Description,
					AdapterDesc1.VendorId,
					AdapterDesc1.DeviceId,
					AdapterDesc1.SubSysId,
					AdapterDesc1.Revision,
					AdapterDesc1.DedicatedVideoMemory / (1024 * 1024),
					AdapterDesc1.DedicatedSystemMemory / (1024 * 1024),
					AdapterDesc1.SharedSystemMemory / (1024 * 1024),
				/*DESC.AdapterLuid,*/
					AdapterDesc1.Flags
					);
			}

			IDXGIOutput* DXGIOutput = NULL;
			for (UINT OutputIndex = 0; SUCCEEDED(DXGIAdapter1->EnumOutputs(OutputIndex,&DXGIOutput)); ++OutputIndex)
			{
				DXGI_OUTPUT_DESC OutputDesc;
				if (SUCCEEDED(DXGIOutput->GetDesc(&OutputDesc)))
				{
					X_LOG("\tOutput:%u, DeviceName:%ls, Width:%u Height:%u \n", 
						OutputIndex, 
						OutputDesc.DeviceName, 
						OutputDesc.DesktopCoordinates.right - OutputDesc.DesktopCoordinates.left, 
						OutputDesc.DesktopCoordinates.bottom - OutputDesc.DesktopCoordinates.top);
				}
			}
		}
	}
	DXGIFactor1->Release();
}

void EnumAdapter2()
{

}