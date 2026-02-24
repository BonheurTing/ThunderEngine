#pragma once
#include <d3d12.h>

#include "RHIDefinition.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
	using namespace Microsoft::WRL;
#define MAX_RENDER_TARGETS 8

	class RenderPass;

	struct RHIVertexElement
	{
		RHIVertexElement(ERHIVertexInputSemantic name, uint8 index, RHIFormat format, uint8 inputSlot, uint16 alignedByteOffset, bool isPerInstanceData);
		
		union
		{
			uint8 Hash[5] = { 0, 0, 0, 0, 0 };
			struct
			{
				ERHIVertexInputSemantic Name : 4;
				uint8 Index : 4;
				RHIFormat Format : 8;
				uint16 AlignedByteOffset;
				uint8 InputSlot : 4;
				uint8 IsPerInstanceData : 1;
				uint8 Padding : 3;
			};
		};
	};
	
	struct RHIVertexDeclarationDescriptor
	{
		RHIVertexDeclarationDescriptor() = default;
		RHIVertexDeclarationDescriptor(TArray<RHIVertexElement> const& inElements) : Elements(inElements) {}
		TArray<RHIVertexElement> Elements;
	};

	struct RHIBlendDescriptor
	{
		RHIBlendDescriptor();
		union
		{
			uint8 Hash[5];
			struct
			{
				uint32 BlendEnable : 1;
				uint32 LogicOpEnable : 1;
				ERHIBlend SrcBlend : 5;
				ERHIBlend DestBlend : 5;
				ERHIBlendOp BlendOp : 3;
				ERHIBlend SrcBlendAlpha : 5;
				ERHIBlend DestBlendAlpha : 5;
				ERHIBlendOp BlendOpAlpha : 3;
				ERHILogicOp LogicOp : 4;
				uint8 RenderTargetWriteMask : 4;
				uint8 Padding : 4;
			};
		};
	};

	struct RHIBlendState
	{
		RHIBlendState();
		union
		{
			uint8 Hash[41];
			struct
			{
				RHIBlendDescriptor RenderTarget[MAX_RENDER_TARGETS];
				uint8 AlphaToCoverageEnable : 1;
				uint8 IndependentBlendEnable : 1;
				uint8 Padding : 6;
			};
		};
	};
	
	RHI_API extern THashMap<ERHIDepthBiasType, DepthBiasConfig> GRHIDepthBiasConfig;

	struct RHIRasterizerState
	{
		RHIRasterizerState();
		union
		{
			uint8 Hash[2];
			struct
			{
				ERHIFillMode FillMode : 2;
				ERHICullMode CullMode : 2;
				uint8 FrontCounterClockwise : 1;
				ERHIDepthBiasType DepthBias : 3;
				uint8 DepthClipEnable : 1;
				uint8 MultisampleEnable : 1;
				uint8 AntiAliasedLineEnable : 1;
				uint8 ForcedSampleCount : 4;
				uint8 ConservativeRaster : 1;
				uint8 Padding : 1;
			};
		};
	};

	struct RHIDepthStencilState
	{
		RHIDepthStencilState();
		union
		{
			uint8 Hash[6] = { 0, 0, 0, 0, 0, 0 };

			struct
			{
				// 24 bits.
				ERHIStencilOp FrontFaceStencilFailOp : 3;
				ERHIStencilOp FrontFaceStencilDepthFailOp : 3;
				ERHIStencilOp FrontFaceStencilPassOp : 3;
				ERHIComparisonFunc FrontFaceStencilFunc : 3;
				ERHIStencilOp BackFaceStencilFailOp : 3;
				ERHIStencilOp BackFaceStencilDepthFailOp : 3;
				ERHIStencilOp BackFaceStencilPassOp : 3;
				ERHIComparisonFunc BackFaceStencilFunc : 3;

				// 8 bits.
				uint32 DepthEnable : 1;
				ERHIDepthWriteMask DepthWriteMask: 1;
				ERHIComparisonFunc DepthFunc : 3;
				uint32 StencilEnable : 1;
				uint32 Padding : 2;

				// 16 bits.
				uint8 StencilReadMask : 8;
				uint8 StencilWriteMask : 8;
			};
		};
	};

	struct TGraphicsPipelineStateDescriptor
	{
		TShaderRegisterCounts           RegisterCounts{};
		RHIVertexDeclarationDescriptor	VertexDeclaration{};
		RHIBlendState					BlendState{};
		RHIRasterizerState			    RasterizerState{};
		RHIDepthStencilState            DepthStencilState{};
		RenderPass*						Pass = nullptr;
		ERHIPrimitiveType				PrimitiveType : 3 = ERHIPrimitiveType::Triangle;
		uint8							NumSamples : 4 = 1;
		class ShaderCombination*		shaderVariant = nullptr;

		void GetStateIdentifier(TArray<uint8>& outIdentifier) const;
		TGraphicsPipelineStateDescriptor() = default;
	};
	struct TComputePipelineStateDescriptor
	{
		
	};
	
	/**
	 * rhi resource
	 */
	
	struct RHIViewDescriptor
	{
		RHIFormat Format;
		ERHIViewDimension Type;
		uint64 Width;
		uint32 Height;
		uint16 DepthOrArraySize;
	};

	struct RHISamplerDescriptor
	{
		ERHIFilter Filter = ERHIFilter::MinMagMipPoint;
		ERHITextureAddressMode AddressU = ERHITextureAddressMode::Wrap;
		ERHITextureAddressMode AddressV = ERHITextureAddressMode::Wrap;
		ERHITextureAddressMode AddressW = ERHITextureAddressMode::Wrap;
		float MipLODBias = 0.f;
		UINT MaxAnisotropy = 8;
		ERHICompareFunction ComparisonFunc = ERHICompareFunction::Never;
		float BorderColor[4] = {};
		float MinLOD = 0.f;
		float MaxLOD = FLOAT_MAX;
	};
    RHI_API extern const TArray<RHISamplerDescriptor> GStaticSamplerDefinitions;
    
    struct RHIResourceFlags
    {
    	uint8 DenySRV : 1;
    	uint8 NeedCBV : 1;
    	uint8 NeedUAV : 1;
    	uint8 NeedRTV : 1;
    	uint8 NeedDSV : 1;
    };
    
	struct RHIResourceSampleDescriptor
	{
		UINT Count;
		UINT Quality;

		RHIResourceSampleDescriptor() : Count(0), Quality(0) {}
		RHIResourceSampleDescriptor(UINT inCount, UINT inQuality) :	Count(inCount), Quality(inQuality) {}
	};
	
    struct RHIResourceDescriptor
    {
    	RHIResourceDescriptor() = default;
    
    	RHIResourceDescriptor(
    		ERHIResourceType dimension,
    		UINT64 alignment,
    		UINT64 width,
    		UINT height,
    		UINT16 depthOrArraySize,
    		UINT16 mipLevels,
    		RHIFormat format,
    		UINT sampleCount,
    		UINT sampleQuality,
    		ERHITextureLayout layout,
    		RHIResourceFlags flags) noexcept
    		: SampleDesc{}, Flags{}
    	{
    		Type = dimension;
    		Alignment = alignment;
    		Width = width;
    		Height = height;
    		DepthOrArraySize = depthOrArraySize;
    		MipLevels = mipLevels;
    		Format = format;
    		SampleDesc.Count = sampleCount;
    		SampleDesc.Quality = sampleQuality;
    		Layout = layout;
    		Flags = flags;
    	}
    
    	static inline RHIResourceDescriptor Buffer(
    		UINT64 width,
    		RHIResourceFlags flags = {},
    		UINT64 alignment = 0) noexcept
    	{
    		return RHIResourceDescriptor( ERHIResourceType::Buffer, alignment, width, 1, 1, 1,
    			RHIFormat::UNKNOWN, 1, 0, ERHITextureLayout::RowMajor, flags );
    	}
    
    	ERHIResourceType Type;
    	UINT64 Alignment;
    	UINT64 Width;
    	UINT Height;
    	UINT16 DepthOrArraySize;
    	UINT16 MipLevels;
    	RHIFormat Format;
    	RHIResourceSampleDescriptor SampleDesc;
    	ERHITextureLayout Layout;
    	RHIResourceFlags Flags;

    	// Optional optimized clear value. Only meaningful for RTV/DSV resources.
    	// When bHasOptimizedClearValue is true the driver can perform fast clears.
    	bool  bHasOptimizedClearValue = false;
    	float OptimizedClearColor[4]  = { 0.f, 0.f, 0.f, 1.f }; // used when NeedRTV
    	float OptimizedClearDepth     = 1.f;                      // used when NeedDSV
    	uint8 OptimizedClearStencil   = 0;                        // used when NeedDSV
    };
	
	/**
 * \brief Descriptor View
 */
	class RHIDescriptorView : public RefCountedObject
	{
	public:
		RHI_API RHIDescriptorView(RHIViewDescriptor const& desc, uint64 handle = 0xFFFFFFFFFFFFFFFF, uint32 offlineHeapIndex = 0xFFFFFFFF)
			: Desc(desc), CPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE{ handle }), OfflineHeapIndex(offlineHeapIndex) {}
		RHI_API virtual ~RHIDescriptorView() = default;

		RHI_API void SetOfflineHandle(uint64 handle) { CPUHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ handle }; }
		RHI_API uint64 GetOfflineHandle() const { return static_cast<uint64>(CPUHandle.ptr); }
		RHI_API void SetOnlineHandle(uint64 handle) { GPUHandle = D3D12_GPU_DESCRIPTOR_HANDLE{ handle }; }
		RHI_API uint64 GetOnlineHandle() const { return static_cast<uint64>(GPUHandle.ptr); }

		RHI_API void SetCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { CPUHandle = handle; }
		RHI_API D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return CPUHandle; }
		RHI_API void SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) { GPUHandle = handle; }
		RHI_API D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return GPUHandle; }

	protected:
		RHIViewDescriptor Desc = {};
		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle{ 0xFFFFFFFFFFFFFFFF };
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle{ 0xFFFFFFFFFFFFFFFF };
		uint32 OfflineHeapIndex = 0xFFFFFFFF; // Used for freeing this descriptor and return it back to descriptor pool.
	};
    
	class RHIConstantBufferView : public RHIDescriptorView
	{
	public:
		RHIConstantBufferView(RHIViewDescriptor const& desc, uint64 handle = 0xFFFFFFFFFFFFFFFF, uint32 offlineHeapIndex = 0xFFFFFFFF)
			: RHIDescriptorView(desc, handle, offlineHeapIndex) {}
	};
	class RHIShaderResourceView : public RHIDescriptorView
	{
	public:
		RHIShaderResourceView(RHIViewDescriptor const& desc, uint64 handle = 0xFFFFFFFFFFFFFFFF, uint32 offlineHeapIndex = 0xFFFFFFFF)
			: RHIDescriptorView(desc, handle, offlineHeapIndex) {}
	};
	class RHIUnorderedAccessView : public RHIDescriptorView
	{
	public:
		RHIUnorderedAccessView(RHIViewDescriptor const& desc, uint64 handle = 0xFFFFFFFFFFFFFFFF, uint32 offlineHeapIndex = 0xFFFFFFFF)
			: RHIDescriptorView(desc, handle, offlineHeapIndex) {}
	};
	class RHIRenderTargetView : public RHIDescriptorView
	{
	public:
		RHIRenderTargetView(RHIViewDescriptor const& desc, uint64 handle = 0xFFFFFFFFFFFFFFFF, uint32 offlineHeapIndex = 0xFFFFFFFF)
			: RHIDescriptorView(desc, handle, offlineHeapIndex) {}
	};
	class RHIDepthStencilView : public RHIDescriptorView
	{
	public:
		RHIDepthStencilView(RHIViewDescriptor const& desc, uint64 handle = 0xFFFFFFFFFFFFFFFF, uint32 offlineHeapIndex = 0xFFFFFFFF)
			: RHIDescriptorView(desc, handle, offlineHeapIndex) {}
	};

	class RHISampler : public RefCountedObject
	{
	public:
		RHI_API RHISampler(RHISamplerDescriptor const& desc) : Desc(desc) {}

	protected:
		RHISamplerDescriptor Desc = {};
	};

	class RHIFence : public RefCountedObject
	{
	public:
		RHI_API RHIFence(uint64 initValue) : Value(initValue) {}
	private:
		uint64 Value;
	};
}
