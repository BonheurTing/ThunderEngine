#pragma once

#include "IDynamicRHI.h"

namespace Thunder
{
	class D3D11RHI_API D3D11DynamicRHI : public IDynamicRHI
	{
	public:
		D3D11DynamicRHI() {}
		virtual ~D3D11DynamicRHI() = default;
	    
		/////// RHI Methods
		RHIDeviceRef RHICreateDevice() override;

		/*RHIRasterizerStateRef RHICreateRasterizerState(const RasterizerStateInitializerRHI& Initializer) override;

		RHIDepthStencilStateRef RHICreateDepthStencilState(const DepthStencilStateInitializerRHI& Initializer) override;

		RHIBlendStateRef RHICreateBlendState(const BlendStateInitializerRHI& Initializer) override;
		
		RHIInputLayoutRef RHICreateInputLayout(const RHIInputLayoutDescriptor& initializer) override;

		RHIVertexDeclarationRef RHICreateVertexDeclaration(const VertexDeclarationInitializerRHI& Elements) override;

		RHIVertexBufferRef RHICreateVertexBuffer(const RHIResourceDescriptor& desc) override;*/

		void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) override {}
        
		void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) override {}
        
		void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) override {}
        
		void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) override {}
        
		void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) override {}
	};
}