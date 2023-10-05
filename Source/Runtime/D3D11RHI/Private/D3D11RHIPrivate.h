#pragma once
#include "RHI.h"
#include "d3d11.h"

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

	/*class D3D11RHIConstantBufferView : public RHIConstantBufferView
	{
	public:
		D3D11RHIConstantBufferView(RHIViewDescriptor const& desc, ID3D11ConstantBufferView* view)
			: RHIConstantBufferView(desc), ConstantBufferView(view) {}
	private:
		ComPtr<ID3D11ConstantBufferView> ConstantBufferView;
	};*/
}
