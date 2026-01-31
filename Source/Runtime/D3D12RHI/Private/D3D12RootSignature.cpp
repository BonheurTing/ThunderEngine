#include "D3D12RootSignature.h"
#include "d3dx12.h"
#include "CommonUtilities.h"
#include "D3D12RHIModule.h"
#include "D3D12RHIPrivate.h"
#include "ShaderDefinition.h"

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
		D3D12DescriptorSettings const& descriptorSettings = TD3D12RHIModule::GetModule()->GetDescriptorSettings();
		constexpr D3D12_ROOT_PARAMETER_TYPE rootParameterTypePriorityOrder[2] = { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, D3D12_ROOT_PARAMETER_TYPE_CBV };
		uint32 rootParameterCount = 0;
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		CD3DX12_ROOT_PARAMETER1 tableSlots[MaxRootParameters];
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRanges[MaxRootParameters];

		// For each root parameter type.
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
					descriptorRanges[rootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shaderRC.ShaderResourceCount, 0u, 0, descriptorSettings.SRVDescriptorRangeFlags);
					tableSlots[rootParameterCount].InitAsDescriptorTable(1, &descriptorRanges[rootParameterCount], D3D12_SHADER_VISIBILITY_ALL);
					rootParameterCount++;
				}

				if (shaderRC.SamplerCount > 0)
				{
					TAssert(rootParameterCount < MaxRootParameters);
					descriptorRanges[rootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, shaderRC.SamplerCount, 0u, 0, descriptorSettings.SamplerDescriptorRangeFlags);
					tableSlots[rootParameterCount].InitAsDescriptorTable(1, &descriptorRanges[rootParameterCount], D3D12_SHADER_VISIBILITY_ALL);
					rootParameterCount++;
				}

				if (shaderRC.UnorderedAccessCount > 0)
				{
					TAssert(rootParameterCount < MaxRootParameters);
					descriptorRanges[rootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, shaderRC.UnorderedAccessCount, 0u, 0, descriptorSettings.UAVDescriptorRangeFlags);
					tableSlots[rootParameterCount].InitAsDescriptorTable(1, &descriptorRanges[rootParameterCount], D3D12_SHADER_VISIBILITY_ALL);
					rootParameterCount++;
				}

				break;
			}
			case D3D12_ROOT_PARAMETER_TYPE_CBV:
			{
				TAssertf(shaderRC.ConstantBufferCount < MAX_CBS, "Too many constant buffers are used, max allowed count is 16.");
				for (uint32 ShaderRegister = 0; (ShaderRegister < shaderRC.ConstantBufferCount) && (ShaderRegister < MAX_CBS); ShaderRegister++)
				{
					TAssert(rootParameterCount < MaxRootParameters);
					tableSlots[rootParameterCount].InitAsConstantBufferView(ShaderRegister, 0, descriptorSettings.CBVRootDescriptorFlags, D3D12_SHADER_VISIBILITY_ALL);
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

