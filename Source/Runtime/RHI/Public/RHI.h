#pragma once
#include "dxgiformat.h"
#include "RHIDefinition.h"

namespace Thunder
{
	struct RHIInputLayoutDescriptor
    {
    	// base
    	LPCSTR                     SemanticName;
    	UINT                       SemanticIndex;
    	RHIFormat                Format;
    	UINT                       InputSlot;
    	UINT                       AlignedByteOffset;
    	ERHIInputClassification    InputSlotClass;
    	UINT                       InstanceDataStepRate;
    	// custom
    };
    
    struct RHISampleDescriptor
    {
    	UINT Count;
    	UINT Quality;
    };
	
	struct RHIViewDescriptor
	{
		RHIFormat Format;
		ERHIViewDimension Type;
	};
    
    struct RHIResourceFlags
    {
    	uint8 NeedSRV : 1;
    	uint8 NeedCBV : 1;
    	uint8 NeedUAV : 1;
    	uint8 NeedRTV : 1;
    	uint8 NeedDSV : 1;
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
    		RHIResourceFlags flags ) noexcept
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
    		RHIResourceFlags flags = {0},
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
    	RHISampleDescriptor SampleDesc;
    	ERHITextureLayout Layout;
    	RHIResourceFlags Flags;
    };
	
    struct VertexDeclarationInitializerRHI
    {
        
    };
    
    struct RasterizerStateInitializerRHI
    {
        
    };
    
    struct DepthStencilStateInitializerRHI
    {
        
    };
    
    struct BlendStateInitializerRHI
    {
        
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
}
