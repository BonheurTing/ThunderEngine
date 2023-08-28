#include "D3D11RHI.h"
#include "D3D11Resource.h"
#include "RHIHelper.h"
#include "D3D11.h"

namespace Thunder
{
	D3D11DynamicRHI::D3D11DynamicRHI() {}

	RHIDeviceRef D3D11DynamicRHI::RHICreateDevice()
	{
		ComPtr<IDXGIFactory4>  factory;
		ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

		D3D_FEATURE_LEVEL  FeatureLevelsRequested = D3D_FEATURE_LEVEL_11_0;
		UINT               numLevelsRequested = 1;
		D3D_FEATURE_LEVEL  FeatureLevelsSupported;
		ComPtr<ID3D11Device> Device;
		ComPtr<ID3D11DeviceContext> context;
		if (SUCCEEDED( D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			&FeatureLevelsRequested,
			numLevelsRequested, 
			D3D11_SDK_VERSION, 
			&Device,
			&FeatureLevelsSupported,
			&context )))
		{
			std::cout << "Succeed to Create D3DDevice (11)" << std::endl;
		}
		
		return RefCountPtr<D3D11Device>{ new D3D11Device{ Device.Get() } };
	}

	RHIRasterizerStateRef D3D11DynamicRHI::RHICreateRasterizerState(const RasterizerStateInitializerRHI& Initializer)
	{
		return nullptr;
	}

	RHIDepthStencilStateRef D3D11DynamicRHI::RHICreateDepthStencilState(const DepthStencilStateInitializerRHI& Initializer)
	{
		return nullptr;
	}

	RHIBlendStateRef D3D11DynamicRHI::RHICreateBlendState(const BlendStateInitializerRHI& Initializer)
	{
		return nullptr;
	}

	RHIInputLayoutRef D3D11DynamicRHI::RHICreateInputLayout(const RHIInputLayoutDescriptor& Initializer)
	{
		return nullptr;
	}

	RHIVertexDeclarationRef D3D11DynamicRHI::RHICreateVertexDeclaration(const VertexDeclarationInitializerRHI& Elements)
	{
		return nullptr;
	}

	RHIPixelShaderRef D3D11DynamicRHI::RHICreatePixelShader()
	{
		return nullptr;
	}

	RHIVertexShaderRef D3D11DynamicRHI::RHICreateVertexShader()
	{
		return nullptr;
	}

	RHIVertexBufferRef D3D11DynamicRHI::RHICreateVertexBuffer(const RHIResourceDescriptor& desc)
	{
		return nullptr;
	}
}