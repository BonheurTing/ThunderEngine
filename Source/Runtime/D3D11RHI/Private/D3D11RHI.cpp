#include "D3D11RHI.h"
#include "D3D11Resource.h"
#include "RHIHelper.h"
#include "D3D11RHIPrivate.h"
#include "D3D11CommandContext.h"
#include "Assertion.h"
#include "d3d11_4.h"
#include "ShaderModule.h"

namespace Thunder
{

	RHIDeviceRef D3D11DynamicRHI::RHICreateDevice()
	{
		ComPtr<IDXGIFactory4>  factory;
		ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

		D3D_FEATURE_LEVEL  FeatureLevelsRequested = D3D_FEATURE_LEVEL_11_0;
		UINT               numLevelsRequested = 1;
		D3D_FEATURE_LEVEL  FeatureLevelsSupported;
		ComPtr<ID3D11DeviceContext> immediateContext;
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
			&immediateContext )))
		{
			std::cout << "Succeed to Create D3DDevice (11)" << std::endl;
		}

		return MakeRefCount<D3D11Device>(Device.Get());
	}

	RHICommandContextRef D3D11DynamicRHI::RHICreateCommandContext()
	{
		ID3D11DeviceContext* deferredContext;
		const HRESULT hr = Device->CreateDeferredContext(0, &deferredContext);
		
		if (SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11CommandContext>(deferredContext);
		}
		return nullptr;
	}

	TRHIGraphicsPipelineState* D3D11DynamicRHI::RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer)
	{
		const TArray<RHIVertexElement>& inElements = initializer.VertexDeclaration.Elements;
		ShaderModule* shaderModule = ShaderModule::GetModule();
		
		ShaderCombination* combination = shaderModule->GetShaderCombination(initializer.ShaderIdentifier.ToString());
		TAssertf(combination != nullptr, "Failed to get shader combination");
		const BinaryData* shaderByteCode = combination->GetByteCode(EShaderStageType::Vertex);
		TAssertf(shaderByteCode->Data != nullptr, "Failed to get vertex shader byte code");
		
		TArray<D3D11_INPUT_ELEMENT_DESC> descs;
		for(const RHIVertexElement& element : inElements)
		{
			D3D11_INPUT_ELEMENT_DESC D3DElement{};
			TAssertf(GVertexInputSemanticToString.contains(element.Name), "Unknown vertex element semantic");
			D3DElement.SemanticName = GVertexInputSemanticToString.find(element.Name)->second.c_str();
			D3DElement.SemanticIndex = element.Index;
			D3DElement.Format = static_cast<DXGI_FORMAT>(element.Format);
			D3DElement.InputSlot = element.InputSlot;
			D3DElement.AlignedByteOffset = element.AlignedByteOffset;
			D3DElement.InputSlotClass = element.IsPerInstanceData ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
			D3DElement.InstanceDataStepRate = 0;
			descs.push_back(D3DElement);
		}
		TAssertf(inElements.size() < MaxVertexElementCount, "Too many vertex elements in declaration");
		ID3D11InputLayout* inputLayout;
		const HRESULT hr = Device->CreateInputLayout(descs.data(), static_cast<UINT>(inElements.size()), shaderByteCode->Data, shaderByteCode->Size, &inputLayout);
		if (SUCCEEDED(hr))
		{
			//todo return MakeRefCount<D3D11RHIVertexDeclaration>(inElements, inputLayout);
		}
		return nullptr;
	}

	void D3D11DynamicRHI::RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize)
	{
		TAssertf(false, "Not implemented");
	}

	void D3D11DynamicRHI::RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
		SRVDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.Type);
		const auto resourceDesc = resource.GetResourceDescriptor();
		switch (resourceDesc->Type)
		{
		case ERHIResourceType::Buffer:
			SRVDesc.Buffer.FirstElement = 0;
			SRVDesc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
			break;
		case ERHIResourceType::Texture1D:
			SRVDesc.Texture1D.MostDetailedMip = 0;
			SRVDesc.Texture1D.MipLevels = -1;
			break;
		case ERHIResourceType::Texture2D:
			SRVDesc.Texture2D.MostDetailedMip = 0;
			SRVDesc.Texture2D.MipLevels = -1;
			break;
		case ERHIResourceType::Texture2DArray:
			//todo: detail desc
			SRVDesc.Texture2DArray.MostDetailedMip = 0;
			SRVDesc.Texture2DArray.MipLevels = -1;
			SRVDesc.Texture2DArray.FirstArraySlice = 0;
			SRVDesc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
			break;
		case ERHIResourceType::Texture3D:
			SRVDesc.Texture3D.MostDetailedMip = 0;
			SRVDesc.Texture3D.MipLevels = -1;
			break;
		case ERHIResourceType::Unknown: break;
		}
		
		ID3D11ShaderResourceView* SRVView;
		const auto inst = static_cast<ID3D11Resource*>( resource.GetResource() );
		const HRESULT hr = Device->CreateShaderResourceView(inst, &SRVDesc, &SRVView);

		if(SUCCEEDED(hr))
		{
			D3D11RHIShaderResourceView* view = new D3D11RHIShaderResourceView(desc, SRVView);
			resource.SetSRV(view); 
		}
		else
		{
			TAssertf(false, "Fail to create shader resource view");
		}
	}

	void D3D11DynamicRHI::RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
		UAVDesc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
		const auto resourceDesc = resource.GetResourceDescriptor();
		switch (resourceDesc->Type)
		{
		case ERHIResourceType::Buffer:
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			UAVDesc.Buffer.FirstElement = 0;
			UAVDesc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
			//todo: D3D11_BUFFER_UAV_FLAG
			break;
		case ERHIResourceType::Texture1D:
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
			UAVDesc.Texture1D.MipSlice = 0;
			break;
		case ERHIResourceType::Texture2D:
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			UAVDesc.Texture2D.MipSlice = 0;
			break;
		case ERHIResourceType::Texture2DArray:
			//todo: detail desc
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			UAVDesc.Texture2DArray.MipSlice = 0;
			UAVDesc.Texture2DArray.FirstArraySlice = 0;
			UAVDesc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
			break;
		case ERHIResourceType::Texture3D:
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
			UAVDesc.Texture3D.MipSlice = 0;
			UAVDesc.Texture3D.FirstWSlice = 0;
			UAVDesc.Texture3D.WSize = static_cast<UINT>(resourceDesc->Width);
			break;
		case ERHIResourceType::Unknown: break;
		}
		
		ID3D11UnorderedAccessView* UAVView;
		const auto inst = static_cast<ID3D11Resource*>( resource.GetResource() );
		const HRESULT hr = Device->CreateUnorderedAccessView(inst, &UAVDesc, &UAVView);

		if(SUCCEEDED(hr))
		{
			D3D11RHIUnorderedAccessView* view = new D3D11RHIUnorderedAccessView(desc, UAVView);
			resource.SetUAV(view);
		}
		else
		{
			TAssertf(false, "Fail to create unordered access view");
		}
	}

	void D3D11DynamicRHI::RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc)
	{
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{};
		RTVDesc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
		RTVDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.Type);
		const auto resourceDesc = resource.GetResourceDescriptor();
		switch (resourceDesc->Type)
		{
		case ERHIResourceType::Buffer:
			RTVDesc.Buffer.FirstElement = 0;
			RTVDesc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
			//todo: D3D11_BUFFER_UAV_FLAG
			break;
		case ERHIResourceType::Texture1D:
			RTVDesc.Texture1D.MipSlice = 0;
			break;
		case ERHIResourceType::Texture2D:
			RTVDesc.Texture2D.MipSlice = 0;
			break;
		case ERHIResourceType::Texture2DArray:
			//todo: detail desc
			RTVDesc.Texture2DArray.MipSlice = 0;
			RTVDesc.Texture2DArray.FirstArraySlice = 0;
			RTVDesc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
			break;
		case ERHIResourceType::Texture3D:
			RTVDesc.Texture3D.MipSlice = 0;
			RTVDesc.Texture3D.FirstWSlice = 0;
			RTVDesc.Texture3D.WSize = static_cast<UINT>(resourceDesc->Width);
			break;
		case ERHIResourceType::Unknown: break;
		}
		
		ID3D11RenderTargetView* RTVView;
		const auto inst = static_cast<ID3D11Resource*>( resource.GetResource() );
		const HRESULT hr = Device->CreateRenderTargetView(inst, &RTVDesc, &RTVView);

		if(SUCCEEDED(hr))
		{
			D3D11RHIRenderTargetView* view = new D3D11RHIRenderTargetView(desc, RTVView);
			resource.SetRTV(view);
		}
		else
		{
			TAssertf(false, "Fail to create render target view");
		}
	}

	void D3D11DynamicRHI::RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc{};
		DSVDesc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
		const auto resourceDesc = resource.GetResourceDescriptor();
		switch (resourceDesc->Type)
		{
		case ERHIResourceType::Buffer:
			break;
		case ERHIResourceType::Texture1D:
			DSVDesc.Texture1D.MipSlice = 0;
			DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
			break;
		case ERHIResourceType::Texture2D:
			DSVDesc.Texture2D.MipSlice = 0;
			DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			break;
		case ERHIResourceType::Texture2DArray:
			//todo: detail desc
			DSVDesc.Texture2DArray.MipSlice = 0;
			DSVDesc.Texture2DArray.FirstArraySlice = 0;
			DSVDesc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
			DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			break;
		case ERHIResourceType::Texture3D:
		case ERHIResourceType::Unknown:
			break;
		}
		
		ID3D11DepthStencilView* DSVView;
		const auto inst = static_cast<ID3D11Resource*>( resource.GetResource() );
		const HRESULT hr = Device->CreateDepthStencilView(inst, &DSVDesc, &DSVView);

		if(SUCCEEDED(hr))
		{
			D3D11RHIDepthStencilView* view = new D3D11RHIDepthStencilView(desc, DSVView);
			resource.SetDSV(view);
		}
		else
		{
			TAssertf(false, "Fail to create render target view");
		}
	}

	RHISamplerRef D3D11DynamicRHI::RHICreateSampler(const RHISamplerDescriptor& desc)
	{
		D3D11_SAMPLER_DESC samplerDesc;
		samplerDesc.Filter = static_cast<D3D11_FILTER>(desc.Filter);
		samplerDesc.AddressU = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(desc.AddressU);
		samplerDesc.AddressV = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(desc.AddressV);
		samplerDesc.AddressW = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(desc.AddressW);
		samplerDesc.MipLODBias = desc.MipLODBias;
		samplerDesc.MaxAnisotropy = desc.MaxAnisotropy;
		samplerDesc.ComparisonFunc = static_cast<D3D11_COMPARISON_FUNC>(desc.ComparisonFunc);
		samplerDesc.BorderColor[0] = desc.BorderColor[0];
		samplerDesc.BorderColor[1] = desc.BorderColor[1];
		samplerDesc.BorderColor[2] = desc.BorderColor[2];
		samplerDesc.BorderColor[3] = desc.BorderColor[3];
		samplerDesc.MinLOD = desc.MinLOD;
		samplerDesc.MaxLOD = desc.MaxLOD;

		ID3D11SamplerState *sampler;
		const HRESULT hr = Device->CreateSamplerState(&samplerDesc,  &sampler);
		if(SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHISampler>(desc, sampler);
		}
		else
		{
			TAssertf(false, "Fail to create sampler");
			return nullptr;
		}
	}
	
	RHIFenceRef D3D11DynamicRHI::RHICreateFence(uint64 initValue, uint32 fenceFlags)
	{
		TAssert(Device != nullptr);
		ComPtr<ID3D11Fence> fence;
		ID3D11Device5* device5;
		HRESULT hr = Device->QueryInterface<ID3D11Device5>(&device5);
		if (SUCCEEDED(hr))
		{
			D3D11_FENCE_FLAG dx11Flags = D3D11_FENCE_FLAG_NONE;
			if (fenceFlags & static_cast<uint64>(ERHIFenceFlags::Shared))
			{
				dx11Flags |= D3D11_FENCE_FLAG_SHARED;
			}
			if (fenceFlags & static_cast<uint64>(ERHIFenceFlags::SharedCrossAdapter))
			{
				dx11Flags |= D3D11_FENCE_FLAG_SHARED_CROSS_ADAPTER;
			}
			if (fenceFlags & static_cast<uint64>(ERHIFenceFlags::NonMonitored))
			{
				dx11Flags |= D3D11_FENCE_FLAG_NON_MONITORED;
			}
			
			hr = device5->CreateFence(initValue, dx11Flags, IID_PPV_ARGS(&fence));
		}

		if(SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHIFence>(initValue, fence);
		}
		else
		{
			TAssertf(false, "Fail to create fence");
			return nullptr;
		}
	}

	RHIVertexBufferRef D3D11DynamicRHI::RHICreateVertexBuffer(uint32 sizeInBytes, uint32 StrideInBytes,  EResourceUsageFlags usage, void *resourceData)
	{
		D3D11_BUFFER_DESC vbDesc;
		vbDesc.ByteWidth = sizeInBytes;
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		vbDesc.CPUAccessFlags = 0;
		if (EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic))
		{
			vbDesc.Usage = D3D11_USAGE_DYNAMIC;
			vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc.MiscFlags = 0;
		vbDesc.StructureByteStride = 0;

		const D3D11_SUBRESOURCE_DATA initData = {resourceData, 0, 0};

		ID3D11Buffer* vertexBuffer;
		const HRESULT hr = Device->CreateBuffer(&vbDesc, resourceData ? &initData : nullptr, &vertexBuffer);

		if (SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHIVertexBuffer>(RHIResourceDescriptor::Buffer(sizeInBytes), vertexBuffer);
		}
		else
		{
			TAssertf(false, "Fail to create vertex buffer");
			return nullptr;
		}
	}

	RHIIndexBufferRef D3D11DynamicRHI::RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, EResourceUsageFlags usage, void *resourceData)
	{
		D3D11_BUFFER_DESC ibDesc;
		ibDesc.ByteWidth = width;
		ibDesc.Usage = D3D11_USAGE_DEFAULT;
		ibDesc.CPUAccessFlags = 0;
		if (EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic))
		{
			ibDesc.Usage = D3D11_USAGE_DYNAMIC;
			ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibDesc.MiscFlags = 0;
		ibDesc.StructureByteStride = 0;

		const D3D11_SUBRESOURCE_DATA initData = {resourceData, 0, 0};

		ID3D11Buffer* indexBuffer;
		const HRESULT hr = Device->CreateBuffer(&ibDesc, resourceData ? &initData : nullptr, &indexBuffer);

		if (SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHIIndexBuffer>(RHIResourceDescriptor::Buffer(width), indexBuffer);
		}
		else
		{
			TAssertf(false, "Fail to create vertex buffer");
			return nullptr;
		}
	}

	RHIStructuredBufferRef D3D11DynamicRHI::RHICreateStructuredBuffer(uint32 size,  EResourceUsageFlags usage, void *resourceData)
	{
		D3D11_BUFFER_DESC sbDesc;
		sbDesc.ByteWidth = size;
		sbDesc.Usage = D3D11_USAGE_DEFAULT;
		sbDesc.CPUAccessFlags = 0;
		if (EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic))
		{
			sbDesc.Usage = D3D11_USAGE_DYNAMIC;
			sbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		sbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		sbDesc.MiscFlags = 0;
		sbDesc.StructureByteStride = 0;

		const D3D11_SUBRESOURCE_DATA initData = {resourceData, 0, 0};

		ID3D11Buffer* structuredBuffer;
		const HRESULT hr = Device->CreateBuffer(&sbDesc, resourceData ? &initData : nullptr, &structuredBuffer);

		if (SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHIStructuredBuffer>(RHIResourceDescriptor::Buffer(size), structuredBuffer);
		}
		else
		{
			TAssertf(false, "Fail to create vertex buffer");
			return nullptr;
		}
	}

	RHIConstantBufferRef D3D11DynamicRHI::RHICreateConstantBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData)
	{
		D3D11_BUFFER_DESC sbDesc;
		sbDesc.ByteWidth = size;
		sbDesc.Usage = D3D11_USAGE_DEFAULT;
		sbDesc.CPUAccessFlags = 0;
		if (EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic))
		{
			sbDesc.Usage = D3D11_USAGE_DYNAMIC;
			sbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		sbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		sbDesc.MiscFlags = 0;
		sbDesc.StructureByteStride = 0;

		const D3D11_SUBRESOURCE_DATA initData = {resourceData, 0, 0};

		ID3D11Buffer* structuredBuffer;
		const HRESULT hr = Device->CreateBuffer(&sbDesc, resourceData ? &initData : nullptr, &structuredBuffer);

		if (SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHIConstantBuffer>(RHIResourceDescriptor::Buffer(size), structuredBuffer);
		}
		else
		{
			TAssertf(false, "Fail to create vertex buffer");
			return nullptr;
		}
	}

	RHITexture1DRef D3D11DynamicRHI::RHICreateTexture1D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData)
	{
		ID3D11Texture1D* texture;
		TAssertf(desc.Type == ERHIResourceType::Texture1D, "Type should be Texture1D.");
		TAssertf(desc.Height == 1, "Height should be 1 for Texture1D.");
		TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Texture1D.");
		D3D11_TEXTURE1D_DESC d3d11Desc = {
			static_cast<UINT>(desc.Width),
			1,
			1,
			ConvertRHIFormatToD3DFormat(desc.Format),
			D3D11_USAGE_DEFAULT,
			GetRHIResourceBindFlags(desc.Flags),
			0,
			0
		};
		if (EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic))
		{
			d3d11Desc.Usage = D3D11_USAGE_DYNAMIC;
			d3d11Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}

		const D3D11_SUBRESOURCE_DATA initData = {resourceData, 0, 0};
		const HRESULT hr = Device->CreateTexture1D(&d3d11Desc, resourceData ? &initData : nullptr, &texture);
    
		if(SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHITexture1D>(desc, texture);
		}
		else
		{
			TAssertf(false, "Fail to create texture1d");
			return nullptr;
		}
	}

	RHITexture2DRef D3D11DynamicRHI::RHICreateTexture2D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData)
	{
		ID3D11Texture2D* texture;
		TAssertf(desc.Type == ERHIResourceType::Texture2D, "Type should be Texture2D.");
		TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Texture12D.");
		D3D11_TEXTURE2D_DESC d3d11Desc = {
			static_cast<UINT>(desc.Width),
			desc.Height,
			desc.MipLevels,
			1,
			ConvertRHIFormatToD3DFormat(desc.Format),
			{1,0},
			D3D11_USAGE_DEFAULT,
			GetRHIResourceBindFlags(desc.Flags),
			0,
			0
		};
		if (EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic))
		{
			d3d11Desc.Usage = D3D11_USAGE_DYNAMIC;
			d3d11Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		
		const D3D11_SUBRESOURCE_DATA initData = {resourceData, 0, 0};
		const HRESULT hr = Device->CreateTexture2D(&d3d11Desc, resourceData ? &initData : nullptr, &texture);
    
		if(SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHITexture2D>(desc, texture);
		}
		else
		{
			TAssertf(false, "Fail to create texture2d");
			return nullptr;
		}
	}

	RHITexture2DArrayRef D3D11DynamicRHI::RHICreateTexture2DArray(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData)
	{
		ID3D11Texture2D* texture;
		TAssertf(desc.Type == ERHIResourceType::Texture2DArray, "Type should be Texture2DArray.");
		D3D11_TEXTURE2D_DESC d3d11Desc = {
			static_cast<UINT>(desc.Width),
			desc.Height,
			desc.MipLevels,
			desc.DepthOrArraySize,
			ConvertRHIFormatToD3DFormat(desc.Format),
			{1,0},
			D3D11_USAGE_DEFAULT,
			GetRHIResourceBindFlags(desc.Flags),
			0,
			0
		};
		if (EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic))
		{
			d3d11Desc.Usage = D3D11_USAGE_DYNAMIC;
			d3d11Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		
		const D3D11_SUBRESOURCE_DATA initData = {resourceData, 0, 0};
		const HRESULT hr = Device->CreateTexture2D(&d3d11Desc, resourceData ? &initData : nullptr, &texture);
    
		if(SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHITexture2DArray>(desc, texture);
		}
		else
		{
			TAssertf(false, "Fail to create texture2dArray");
			return nullptr;
		}
	}

	RHITexture3DRef D3D11DynamicRHI::RHICreateTexture3D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData)
	{
		ID3D11Texture3D* texture;
		TAssertf(desc.Type == ERHIResourceType::Texture3D, "Type should be Texture3D.");
		D3D11_TEXTURE3D_DESC d3d11Desc = {
			static_cast<UINT>(desc.Width),
			desc.Height,
			desc.DepthOrArraySize,
			desc.MipLevels,
			ConvertRHIFormatToD3DFormat(desc.Format),
			D3D11_USAGE_DEFAULT,
			GetRHIResourceBindFlags(desc.Flags),
			0,
			0
		};
		if (EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic))
		{
			d3d11Desc.Usage = D3D11_USAGE_DYNAMIC;
			d3d11Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		
		const D3D11_SUBRESOURCE_DATA initData = {resourceData, 0, 0};
		const HRESULT hr = Device->CreateTexture3D(&d3d11Desc, resourceData ? &initData : nullptr, &texture);
    
		if(SUCCEEDED(hr))
		{
			return MakeRefCount<D3D11RHITexture3D>(desc, texture);
		}
		else
		{
			TAssertf(false, "Fail to create texture3d");
			return nullptr;
		}
	}

	bool D3D11DynamicRHI::RHIUpdateSharedMemoryResource(RHIResource* resource, void* resourceData, uint32 size, uint8 subresourceId)
	{
		TAssertf(false, "Not implemented");
		return false;
	}
}
