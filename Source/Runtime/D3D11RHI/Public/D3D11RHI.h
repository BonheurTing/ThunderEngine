#pragma once

#include "DynamicRHI.h"

namespace Thunder
{
	class D3D11RHI_API D3D11DynamicRHI : public DynamicRHI
	{
	public:
		D3D11DynamicRHI();
		virtual ~D3D11DynamicRHI() = default;
	    
		/////// RHI Methods
		RHIDeviceRef RHICreateDevice() override;

		RHIRasterizerStateRef RHICreateRasterizerState(const RasterizerStateInitializerRHI& Initializer) override;

		RHIDepthStencilStateRef RHICreateDepthStencilState(const DepthStencilStateInitializerRHI& Initializer) override;

		RHIBlendStateRef RHICreateBlendState(const BlendStateInitializerRHI& Initializer) override;
		
		RHIInputLayoutRef RHICreateInputLayout(const RHIInputLayoutDescriptor& initializer) override;

		RHIVertexDeclarationRef RHICreateVertexDeclaration(const VertexDeclarationInitializerRHI& Elements) override;

		RHIPixelShaderRef RHICreatePixelShader() override;

		RHIVertexShaderRef RHICreateVertexShader() override;

		RHIVertexBufferRef RHICreateVertexBuffer(const RHIResourceDescriptor& desc) override;
	};
}