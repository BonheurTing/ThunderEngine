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

		RHIConstantBufferViewRef RHICreateConstantBufferView(const RHIViewDescriptor& desc) override {return nullptr;}
        
		void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) override {}
        
		RHIUnorderedAccessViewRef RHICreateUnorderedAccessView(const RHIViewDescriptor& desc) override {return nullptr;}
        
		RHIRenderTargetViewRef RHICreateRenderTargetView(const RHIViewDescriptor& desc) override {return nullptr;}
        
		RHIDepthStencilViewRef RHICreateDepthStencilView(const RHIViewDescriptor& desc) override {return nullptr;}
	};
}