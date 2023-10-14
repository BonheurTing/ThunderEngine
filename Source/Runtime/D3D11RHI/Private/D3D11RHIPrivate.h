#pragma once
#include "RHI.h"
#include "d3d11.h"
#include "d3d11_3.h"

namespace Thunder
{
	class D3D11RHIShaderResourceView : public RHIShaderResourceView
	{
	public:
		D3D11RHIShaderResourceView(RHIViewDescriptor const& desc, ID3D11ShaderResourceView* view)
			: RHIShaderResourceView(desc), ShaderResourceView(view) {}
	private:
		ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
	};

	class D3D11RHIUnorderedAccessView : public RHIUnorderedAccessView
	{
	public:
		D3D11RHIUnorderedAccessView(RHIViewDescriptor const& desc, ID3D11UnorderedAccessView* view)
			: RHIUnorderedAccessView(desc), UnorderedAccessView(view) {}
	private:
		ComPtr<ID3D11UnorderedAccessView> UnorderedAccessView;
	};

	class D3D11RHIRenderTargetView : public RHIRenderTargetView
	{
	public:
		D3D11RHIRenderTargetView(RHIViewDescriptor const& desc, ID3D11RenderTargetView* view)
			: RHIRenderTargetView(desc), RenderTargetView(view) {}
	private:
		ComPtr<ID3D11RenderTargetView> RenderTargetView;
	};

	class D3D11RHIDepthStencilView : public RHIDepthStencilView
	{
	public:
		D3D11RHIDepthStencilView(RHIViewDescriptor const& desc, ID3D11DepthStencilView* view)
			: RHIDepthStencilView(desc), DepthStencilView(view) {}
	private:
		ComPtr<ID3D11DepthStencilView> DepthStencilView;
	};

	class D3D11RHISampler : public RHISampler
	{
	public:
		D3D11RHISampler(RHISamplerDescriptor const& desc,  ID3D11SamplerState* sampler)
			: RHISampler(desc), SamplerState(sampler) {}

	private:
		ComPtr<ID3D11SamplerState> SamplerState;
	};

	class D3D11RHIFence : public RHIFence
	{
	public:
		D3D11RHIFence(uint64 initValue, ComPtr<ID3D11Fence> const& inFence)
			: RHIFence(initValue), Fence(inFence) {}
		
	private:
		ComPtr<ID3D11Fence> Fence;
	};
}
