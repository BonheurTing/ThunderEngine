#pragma once
#include "dxgiformat.h"
#include "RHIDefinition.h"
#include <wrl/client.h>

namespace Thunder
{
	using namespace Microsoft::WRL;
#define MAX_RENDER_TARGETS 8
	
	struct RHIVertexElement
	{
		RHIVertexElement(ERHIVertexInputSemantic name, uint8 index, RHIFormat format, uint8 inputSlot, uint16 alignedByteOffset, bool isPerInstanceData)
			: Name(name), Index(index), Format(format), InputSlot(inputSlot), AlignedByteOffset(alignedByteOffset), IsPerInstanceData(isPerInstanceData) {}
		
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
				uint8 BlendEnable : 1;
				uint8 LogicOpEnable : 1;
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
				uint8 DepthEnable : 1;
				ERHIDepthWriteMask DepthWriteMask: 1;
				ERHIComparisonFunc DepthFunc : 3;
				uint8 StencilEnable : 1;
				uint8 Padding : 1;
				uint8 StencilReadMask : 8;
				uint8 StencilWriteMask : 8;

				ERHIStencilOp FrontFaceStencilFailOp : 3;
				ERHIStencilOp FrontFaceStencilDepthFailOp : 3;
				ERHIStencilOp FrontFaceStencilPassOp : 3;
				ERHIComparisonFunc FrontFaceStencilFunc : 3;
				ERHIStencilOp BackFaceStencilFailOp : 3;
				ERHIStencilOp BackFaceStencilDepthFailOp : 3;
				ERHIStencilOp BackFaceStencilPassOp : 3;
				ERHIComparisonFunc BackFaceStencilFunc : 3;
			};
		};
	};

	struct TGraphicsPipelineStateDescriptor
	{
		TShaderRegisterCounts           RegisterCounts;
		RHIVertexDeclarationDescriptor	VertexDeclaration;
		RHIBlendState					BlendState;
		RHIRasterizerState			    RasterizerState;
		RHIDepthStencilState            DepthStencilState;
		RHIFormat	                    RenderTargetFormats[MAX_RENDER_TARGETS];
		RHIFormat                       DepthStencilFormat : 8;
		ERHIPrimitiveType				PrimitiveType : 3;
		uint8							RenderTargetsEnabled : 1;
		uint8							NumSamples : 4;

		NameHandle						ShaderIdentifier;
		
		void GetStateIdentifier(TArray<uint8>& outIdentifier) const;
		TGraphicsPipelineStateDescriptor() :
			RegisterCounts{},
			VertexDeclaration{},
			BlendState{},
			RasterizerState{},
			DepthStencilState{},
			RenderTargetFormats{RHIFormat::UNKNOWN},
			DepthStencilFormat{RHIFormat::UNKNOWN},
			PrimitiveType{ERHIPrimitiveType::Undefined},
			RenderTargetsEnabled{0},
			NumSamples{1},
			ShaderIdentifier{""}
		{}
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
		ERHIFilter Filter;
		ERHITextureAddressMode AddressU;
		ERHITextureAddressMode AddressV;
		ERHITextureAddressMode AddressW;
		float MipLODBias;
		UINT MaxAnisotropy;
		ERHICompareFunction ComparisonFunc;
		float BorderColor[4];
		float MinLOD;
		float MaxLOD;
	};
    
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
    };
	
	/**
 * \brief Descriptor View
 */
	class RHI_API RHIDescriptorView
	{
	public:
		RHIDescriptorView(RHIViewDescriptor const& desc) : Desc(desc) {}
		virtual ~RHIDescriptorView() = default;
        
	protected:
		RHIViewDescriptor Desc = {};
	};
    
	class RHIConstantBufferView : public RHIDescriptorView
	{
	public:
		RHIConstantBufferView(RHIViewDescriptor const& desc) : RHIDescriptorView(desc) {}
	};
	class RHIShaderResourceView : public RHIDescriptorView
	{
	public:
		RHIShaderResourceView(RHIViewDescriptor const& desc) : RHIDescriptorView(desc) {}
	};
	class RHIUnorderedAccessView : public RHIDescriptorView
	{
	public:
		RHIUnorderedAccessView(RHIViewDescriptor const& desc) : RHIDescriptorView(desc) {}
	};
	class RHIRenderTargetView : public RHIDescriptorView
	{
	public:
		RHIRenderTargetView(RHIViewDescriptor const& desc) : RHIDescriptorView(desc) {}
	};
	class RHIDepthStencilView : public RHIDescriptorView
	{
	public:
		RHIDepthStencilView(RHIViewDescriptor const& desc) : RHIDescriptorView(desc) {}
	};

	class RHI_API RHISampler
	{
	public:
		RHISampler(RHISamplerDescriptor const& desc) : Desc(desc) {}

	protected:
		RHISamplerDescriptor Desc = {};
	};

	class RHI_API RHIFence
	{
	public:
		RHIFence(uint64 initValue) : Value(initValue) {}
	private:
		uint64 Value;
	};
}
