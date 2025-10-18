#include "D3D12RootSignature.h"
#include "d3dx12.h"
#include "CommonUtilities.h"
#include "D3D12RHIPrivate.h"

namespace Thunder
{
	static D3D12_STATIC_SAMPLER_DESC MakeStaticSampler(D3D12_FILTER Filter, D3D12_TEXTURE_ADDRESS_MODE WrapMode, uint32 Register, uint32 Space)
	{
		D3D12_STATIC_SAMPLER_DESC result = {};
	
		result.Filter           = Filter;
		result.AddressU         = WrapMode;
		result.AddressV         = WrapMode;
		result.AddressW         = WrapMode;
		result.MipLODBias       = 0.0f;
		result.MaxAnisotropy    = 1;
		result.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
		result.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		result.MinLOD           = 0.0f;
		result.MaxLOD           = D3D12_FLOAT32_MAX;
		result.ShaderRegister   = Register;
		result.RegisterSpace    = Space;
		result.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		return result;
	}

	// Static sampler table must match D3DCommon.ush
	static const D3D12_STATIC_SAMPLER_DESC StaticSamplerDescs[] =
	{
		MakeStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT,        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  0, 1000),
		MakeStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT,        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1, 1000),
		MakeStaticSampler(D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP,  2, 1000),
		MakeStaticSampler(D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 3, 1000),
		MakeStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR,       D3D12_TEXTURE_ADDRESS_MODE_WRAP,  4, 1000),
		MakeStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR,       D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 5, 1000),
	};
	
	TD3D12RootSignature::TD3D12RootSignature(ID3D12Device* InParent, const TShaderRegisterCounts& shaderRC) : TD3D12DeviceChild(InParent)
	{
		const D3D12_ROOT_PARAMETER_TYPE rootParameterTypePriorityOrder[2] = { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, D3D12_ROOT_PARAMETER_TYPE_CBV };
		// Determine if our descriptors or their data is static based on the resource binding tier.

		//todo: move to FD3D12Adapter to check device surpport 
		D3D12_RESOURCE_HEAP_TIER resourceHeapTier;
		D3D12_RESOURCE_BINDING_TIER resourceBindingTier;
		D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Caps;
		memset(&d3d12Caps, 0, sizeof(d3d12Caps));
		TAssertf(SUCCEEDED(ParentDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Caps, sizeof(d3d12Caps))), "Failed to check D3D12 feature support.");
		resourceHeapTier = d3d12Caps.ResourceHeapTier;
		resourceBindingTier = d3d12Caps.ResourceBindingTier;

		const D3D12_DESCRIPTOR_RANGE_FLAGS SRVDescriptorRangeFlags = (resourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_1) ?
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE :
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		const D3D12_DESCRIPTOR_RANGE_FLAGS CBVDescriptorRangeFlags = (resourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_2) ?
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE :
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		const D3D12_DESCRIPTOR_RANGE_FLAGS UAVDescriptorRangeFlags = (resourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_2) ?
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE :
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		const D3D12_DESCRIPTOR_RANGE_FLAGS SamplerDescriptorRangeFlags = (resourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_1) ?
			D3D12_DESCRIPTOR_RANGE_FLAG_NONE :
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		const D3D12_ROOT_DESCRIPTOR_FLAGS CBVRootDescriptorFlags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;	// We always set the data in an upload heap before calling Set*RootConstantBufferView.

		
		uint32 rootParameterCount = 0;
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		CD3DX12_ROOT_PARAMETER1 tableSlots[MaxRootParameters];
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRanges[MaxRootParameters];
		
		// For each root parameter type...
		for (uint32 rootParameterTypeIndex = 0; rootParameterTypeIndex < 2; rootParameterTypeIndex++)
		{
			const D3D12_ROOT_PARAMETER_TYPE& rootParameterType = rootParameterTypePriorityOrder[rootParameterTypeIndex];
			switch (rootParameterType)
			{
			case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			{
				if (shaderRC.ShaderResourceCount > 0)
				{
					TAssert(rootParameterCount < MaxRootParameters);
					descriptorRanges[rootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shaderRC.ShaderResourceCount, 0u, 0, SRVDescriptorRangeFlags);
					tableSlots[rootParameterCount].InitAsDescriptorTable(1, &descriptorRanges[rootParameterCount], D3D12_SHADER_VISIBILITY_ALL);
					rootParameterCount++;
				}

				if (shaderRC.ConstantBufferCount > MAX_ROOT_CBVS)
				{
					// Use a descriptor table for the 'excess' CBVs
					TAssert(rootParameterCount < MaxRootParameters);
					descriptorRanges[rootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, shaderRC.ConstantBufferCount - MAX_ROOT_CBVS, MAX_ROOT_CBVS, 0, CBVDescriptorRangeFlags);
					tableSlots[rootParameterCount].InitAsDescriptorTable(1, &descriptorRanges[rootParameterCount], D3D12_SHADER_VISIBILITY_ALL);
					rootParameterCount++;
				}

				if (shaderRC.SamplerCount > 0)
				{
					TAssert(rootParameterCount < MaxRootParameters);
					descriptorRanges[rootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, shaderRC.SamplerCount, 0u, 0, SamplerDescriptorRangeFlags);
					tableSlots[rootParameterCount].InitAsDescriptorTable(1, &descriptorRanges[rootParameterCount], D3D12_SHADER_VISIBILITY_ALL);
					rootParameterCount++;
				}

				if (shaderRC.UnorderedAccessCount > 0)
				{
					TAssert(rootParameterCount < MaxRootParameters);
					descriptorRanges[rootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, shaderRC.UnorderedAccessCount, 0u, 0, UAVDescriptorRangeFlags);
					tableSlots[rootParameterCount].InitAsDescriptorTable(1, &descriptorRanges[rootParameterCount], D3D12_SHADER_VISIBILITY_ALL);
					rootParameterCount++;
				}
				break;
			}
			case D3D12_ROOT_PARAMETER_TYPE_CBV:
			{
				for (uint32 ShaderRegister = 0; (ShaderRegister < shaderRC.ConstantBufferCount) && (ShaderRegister < MAX_ROOT_CBVS); ShaderRegister++)
				{
					TAssert(rootParameterCount < MaxRootParameters);
					tableSlots[rootParameterCount].InitAsConstantBufferView(ShaderRegister, 0, CBVRootDescriptorFlags, D3D12_SHADER_VISIBILITY_ALL);
					rootParameterCount++;
				}
				break;
			}
			default:
				TAssert(false);
				break;
			}
		}
		

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc;
		rootDesc.Init_1_1(rootParameterCount, tableSlots, 6, StaticSamplerDescs, flags);

		ComPtr<ID3DBlob> error;
		ComPtr<ID3DBlob> signature;
		const D3D_ROOT_SIGNATURE_VERSION MaxRootSignatureVersion = rootDesc.Version;
		HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootDesc, MaxRootSignatureVersion, &signature, &error);
		TAssertf(SUCCEEDED(hr), "D3DX12SerializeVersionedRootSignature failed with hrcode %l", hr);
		TAssertf(!error.Get(), "D3DX12SerializeVersionedRootSignature failed with error %s", static_cast<char*>(error->GetBufferPointer()));

		hr = ParentDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
		TAssertf(SUCCEEDED(hr), "D3DX12CreateRootSignature failed with hrcode %l", hr);
	}

	void TD3D12RootSignatureManager::Destroy()
	{
		for (auto Iter = RootSignatureMap.begin(); Iter != RootSignatureMap.end(); ++Iter)
		{
			TMemory::Destroy(Iter->second);
		}
		RootSignatureMap.clear();
	}

	TD3D12RootSignature* TD3D12RootSignatureManager::GetRootSignature(const TShaderRegisterCounts& shaderRC)
	{
		ScopeLock lock(MapMutex);
		if (RootSignatureMap.contains(shaderRC))
		{
			TD3D12RootSignature* pRootSignature = RootSignatureMap[shaderRC];
			return pRootSignature;
		}
		// Create a new root signature and return it.
		return CreateRootSignature(shaderRC);
	}

	TD3D12RootSignature* TD3D12RootSignatureManager::CreateRootSignature(const TShaderRegisterCounts& shaderRC)
	{
		const auto pRootSignature = new (TMemory::Malloc<TD3D12RootSignature>()) TD3D12RootSignature{ParentDevice, shaderRC};
		RootSignatureMap[shaderRC] = pRootSignature;
		return pRootSignature;
	}
}

