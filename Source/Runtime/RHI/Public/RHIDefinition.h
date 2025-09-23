#pragma once
#include "CoreMinimal.h"

namespace Thunder
{
    #define MAX_FRAME_LAG 3
    
    enum class ERHIIndexBufferType : uint8
    {
        Uint16 = 0,
        Uint32
    };

    enum class ERHIVertexInputSemantic : uint8
    {
        Unknown = 0,
        Position,
        TexCoord,
        Normal,
        Tangent,
        Color,
        BiNormal,
        BlendIndices,
        BlendWeights,
        Max
    };
    static const THashMap<ERHIVertexInputSemantic, String> GVertexInputSemanticToString =
    {
        { ERHIVertexInputSemantic::Unknown, "UNKNOWN" },
        { ERHIVertexInputSemantic::Position, "POSITION" },
        { ERHIVertexInputSemantic::TexCoord, "TEXCOORD" },
        { ERHIVertexInputSemantic::Normal, "NORMAL" },
        { ERHIVertexInputSemantic::Tangent, "TANGENT" },
        { ERHIVertexInputSemantic::Color, "COLOR" },
        { ERHIVertexInputSemantic::BiNormal, "BINORMAL" },
        { ERHIVertexInputSemantic::BlendIndices, "BLENDINDICES" },
        { ERHIVertexInputSemantic::BlendWeights, "BLENDWEIGHT" }
    };

    enum class ERHIResourceType : uint8
    {
        Unknown	= 0,
        Buffer,
        Texture1D,
        Texture2D,
        Texture2DArray,
        Texture3D
    };
    
    enum class ERHITextureLayout : uint8
    {
        Unknown = 0,
        RowMajor,
        UndefinedSwizzle64KB,
        StandardSwizzle64KB
    };
    
    enum class RHIFormat : uint8
    {
        UNKNOWN	                                = 0,
        R32G32B32A32_TYPELESS                   = 1,
        R32G32B32A32_FLOAT                      = 2,
        R32G32B32A32_UINT                       = 3,
        R32G32B32A32_SINT                       = 4,
        R32G32B32_TYPELESS                      = 5,
        R32G32B32_FLOAT                         = 6,
        R32G32B32_UINT                          = 7,
        R32G32B32_SINT                          = 8,
        R16G16B16A16_TYPELESS                   = 9,
        R16G16B16A16_FLOAT                      = 10,
        R16G16B16A16_UNORM                      = 11,
        R16G16B16A16_UINT                       = 12,
        R16G16B16A16_SNORM                      = 13,
        R16G16B16A16_SINT                       = 14,
        R32G32_TYPELESS                         = 15,
        R32G32_FLOAT                            = 16,
        R32G32_UINT                             = 17,
        R32G32_SINT                             = 18,
        R32G8X24_TYPELESS                       = 19,
        D32_FLOAT_S8X24_UINT                    = 20,
        R32_FLOAT_X8X24_TYPELESS                = 21,
        X32_TYPELESS_G8X24_UINT                 = 22,
        R10G10B10A2_TYPELESS                    = 23,
        R10G10B10A2_UNORM                       = 24,
        R10G10B10A2_UINT                        = 25,
        R11G11B10_FLOAT                         = 26,
        R8G8B8A8_TYPELESS                       = 27,
        R8G8B8A8_UNORM                          = 28,
        R8G8B8A8_UNORM_SRGB                     = 29,
        R8G8B8A8_UINT                           = 30,
        R8G8B8A8_SNORM                          = 31,
        R8G8B8A8_SINT                           = 32,
        R16G16_TYPELESS                         = 33,
        R16G16_FLOAT                            = 34,
        R16G16_UNORM                            = 35,
        R16G16_UINT                             = 36,
        R16G16_SNORM                            = 37,
        R16G16_SINT                             = 38,
        R32_TYPELESS                            = 39,
        D32_FLOAT                               = 40,
        R32_FLOAT                               = 41,
        R32_UINT                                = 42,
        R32_SINT                                = 43,
        R24G8_TYPELESS                          = 44,
        D24_UNORM_S8_UINT                       = 45,
        R24_UNORM_X8_TYPELESS                   = 46,
        X24_TYPELESS_G8_UINT                    = 47,
        R8G8_TYPELESS                           = 48,
        R8G8_UNORM                              = 49,
        R8G8_UINT                               = 50,
        R8G8_SNORM                              = 51,
        R8G8_SINT                               = 52,
        R16_TYPELESS                            = 53,
        R16_FLOAT                               = 54,
        D16_UNORM                               = 55,
        R16_UNORM                               = 56,
        R16_UINT                                = 57,
        R16_SNORM                               = 58,
        R16_SINT                                = 59,
        R8_TYPELESS                             = 60,
        R8_UNORM                                = 61,
        R8_UINT                                 = 62,
        R8_SNORM                                = 63,
        R8_SINT                                 = 64,
        A8_UNORM                                = 65,
        R1_UNORM                                = 66,
        R9G9B9E5_SHAREDEXP                      = 67,
        R8G8_B8G8_UNORM                         = 68,
        G8R8_G8B8_UNORM                         = 69,
        BC1_TYPELESS                            = 70,
        BC1_UNORM                               = 71,
        BC1_UNORM_SRGB                          = 72,
        BC2_TYPELESS                            = 73,
        BC2_UNORM                               = 74,
        BC2_UNORM_SRGB                          = 75,
        BC3_TYPELESS                            = 76,
        BC3_UNORM                               = 77,
        BC3_UNORM_SRGB                          = 78,
        BC4_TYPELESS                            = 79,
        BC4_UNORM                               = 80,
        BC4_SNORM                               = 81,
        BC5_TYPELESS                            = 82,
        BC5_UNORM                               = 83,
        BC5_SNORM                               = 84,
        B5G6R5_UNORM                            = 85,
        B5G5R5A1_UNORM                          = 86,
        B8G8R8A8_UNORM                          = 87,
        B8G8R8X8_UNORM                          = 88,
        R10G10B10_XR_BIAS_A2_UNORM              = 89,
        B8G8R8A8_TYPELESS                       = 90,
        B8G8R8A8_UNORM_SRGB                     = 91,
        B8G8R8X8_TYPELESS                       = 92,
        B8G8R8X8_UNORM_SRGB                     = 93,
        BC6H_TYPELESS                           = 94,
        BC6H_UF16                               = 95,
        BC6H_SF16                               = 96,
        BC7_TYPELESS                            = 97,
        BC7_UNORM                               = 98,
        BC7_UNORM_SRGB                          = 99,
        AYUV                                    = 100,
        Y410                                    = 101,
        Y416                                    = 102,
        NV12                                    = 103,
        P010                                    = 104,
        P016                                    = 105,
        _420_OPAQUE                             = 106,
        YUY2                                    = 107,
        Y210                                    = 108,
        Y216                                    = 109,
        NV11                                    = 110,
        AI44                                    = 111,
        IA44                                    = 112,
        P8                                      = 113,
        A8P8                                    = 114,
        B4G4R4A4_UNORM                          = 115,
    
        P208                                    = 130,
        V208                                    = 131,
        V408                                    = 132,
    
    
        SAMPLER_FEEDBACK_MIN_MIP_OPAQUE         = 189,
        SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,
    
    
        FORCE_UINT                  = 0xff
    };

    enum class ERHIViewDimension : uint32
    {
        Unknown	= 0,
        Buffer,
        Texture1D,
        Texture1DArray,
        Texture2D,
        Texture2DArray,
        Texture2DMS,
        Texture2DMSArray,
        Texture3D,
        TextureCube,
        TextureCubeArray,
        Bufferex
    };

    enum class ERHIClearFlags : uint8
    {
        None = 0,
        Depth = 1 << 0,
        Stencil = 1 << 1,
        DepthStencil = Depth | Stencil
    };

    enum class EBufferCreateFlags : uint32
    {
        None                    = 0,
        /** The buffer will be written to once. */
        Static                  = 1 << 0,
        /** The buffer will be written to occasionally, GPU read only, CPU write only.  The data lifetime is until the next update, or the buffer is destroyed. */
        Dynamic                 = 1 << 1,
        /** The buffer's data will have a lifetime of one frame.  It MUST be written to each frame, or a new one created each frame. */
        Volatile                = 1 << 2,

        // Helper bit-masks
        AnyDynamic = (Dynamic | Volatile)
    };

    enum class ETextureCreateFlags : uint32
    {
        None                    = 0,
        /** The buffer will be written to once. */
        Static                  = 1 << 0,
        /** The buffer will be written to occasionally, GPU read only, CPU write only.  The data lifetime is until the next update, or the buffer is destroyed. */
        Dynamic                 = 1 << 1,
        /** The buffer's data will have a lifetime of one frame.  It MUST be written to each frame, or a new one created each frame. */
        Volatile                = 1 << 2,

        // Helper bit-masks
        AnyDynamic = (Dynamic | Volatile)
    };

    enum class ERHIFilter : uint32
    {
        MinMagMipPoint = 0,
        MinMagPointMipLinear = 0x1,
        MinPointMagLinearMipPoint = 0x4,
        MinPointMagMipLinear = 0x5,
        MinLinearMagMipPoint = 0x10,
        MinLinearMagPointMipLinear = 0x11,
        MinMagLinearMipPoint = 0x14,
        MinMagMipLinear = 0x15,
        Anisotropic = 0x55,
        ComparisonMinMagMipPoint = 0x80,
        ComparisonMinMagPointMipLinear = 0x81,
        ComparisonPointMagLinearMipPoint = 0x84,
        ComparisonMinPointMagMipLinear = 0x85,
        ComparisonMinLinearMagMipPoint = 0x90,
        ComparisonMinLinearMagPointMipLinear = 0x91,
        ComparisonMinMagLinearMipPoint = 0x94,
        ComparisonMinMagMipLinear = 0x95,
        ComparisonAnisotropic = 0xd5,
        MinimumMinMagMipPoint = 0x100,
        MinimumMinMagPointMipLinear = 0x101,
        MinimumMinPointMagLinearMipPoint = 0x104,
        MinimumMinPointMagMipLinear = 0x105,
        MinimumMinLinearMagMipPoint = 0x110,
        MinimumMinLinearMagPointMipLinear = 0x111,
        MinimumMinMagLinearMipPoint = 0x114,
        MinimumMinMagMipLinear = 0x115,
        MinimumAnisotropic = 0x155,
        MaximumMinMagMipPoint = 0x180,
        MaximumMinMagPointMipLinear = 0x181,
        MaximumMinPointMagLinearMipPoint = 0x184,
        MaximumMinPointMagMipLinear = 0x185,
        MaximumMinLinearMagMipPoint = 0x190,
        MaximumMinLinearMagPointMipLinear = 0x191,
        MaximumMinMagLinearMipPoint = 0x194,
        MaximumMinMagMipLinear = 0x195,
        MaximumAnisotropic = 0x1d5
    };

    enum class ERHITextureAddressMode : uint8
    {
        Wrap = 1,
        Mirror = 2,
        Clamp = 3,
        Border = 4,
        MirrorOnce = 5
    };

    enum class ERHICompareFunction : uint8
    {
        Never = 1,
        Less = 2,
        Equal = 3,
        LessEqual = 4,
        Greater = 5,
        NotEqual = 6,
        GreaterEqual = 7,
        Always = 8
    };

    enum class ERHIFenceFlags : uint8
    {
        None = 0,
        Shared = 0x1,
        SharedCrossAdapter = 0x2,
        NonMonitored = 0x4
    };

    enum class ERHIBlend : uint8
    {
        Zero = 1,
        One	= 2,
        SrcColor = 3,
        InvSrcColor = 4,
        SrcAlpha = 5,
        InvSrcAlpha = 6,
        DestAlpha = 7,
        InvDestAlpha = 8,
        DestColor = 9,
        InvDestColor = 10,
        SrcAlphaSaturate = 11,
        BlendFactor = 14,
        InvBlendFactor = 15,
        Src1Color = 16,
        InvSrc1Color = 17,
        Src1Alpha = 18,
        InvSrc1Alpha = 19,
        AlphaFactor = 20,
        InvAlphaFactor = 21
    };

    enum class ERHIBlendOp : uint8
    {
        Add = 1,
        Subtract = 2,
        RevSubtract = 3,
        Min = 4,
        Max = 5
    };

    enum class ERHILogicOp : uint8
    {
        Clear = 0,
        Set = 1,
        Copy = 2,
        CopyInverted = 3,
        Noop = 4,
        Invert = 5,
        And = 6,
        Nand = 7,
        Or = 8,
        Nor = 9,
        Xor = 10,
        Equiv = 11,
        AndReverse = 12,
        AndInverted = 13,
        OrReverse = 14,
        OrInverted = 15
    };

    enum class ERHIFillMode : uint8
    {
        Wireframe = 2,
        Solid = 3
    };

    enum class ERHICullMode : uint8
    {
        None = 1,
        Front = 2,
        Back = 3
    };

    enum class ERHIDepthBiasType : uint8
    {
        Default = 0,
        Invalid
    };

    struct DepthBiasConfig
    {
        int32 Bias = 0;
        float BiasClamp = 0.f;
        float SlopeScaledBias = 0.f;
    };

    enum class ERHIDepthWriteMask : uint8
    {
        Zero = 0,
        All = 1
    };

    // +1 for the dx12 value
    enum class ERHIComparisonFunc : uint8
    {
        Never = 0,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };

    // +1 for the dx12 value
    enum class ERHIStencilOp : uint8
    {
        Keep = 0,
        Zero,
        Replace,
        IncrSat,
        DecrSat,
        Invert,
        Incr,
        Decr
    };

    enum class ERHIPrimitiveType : uint8
    {
        Undefined = 0,
        Point = 1,
        Line = 2,
        Triangle = 3,
        Patch = 4
    };

    enum class ERHIPrimitive : uint32
    {
        Undefined = 0,
        PointList = 1,
        LineList = 2,
        LineStrip = 3,
        TriangleList = 4,
        TriangleStrip = 5,
        LineListAdj = 10,
        LineStripAdj = 11,
        TriangleListAdj = 12,
        TriangleStripAdj = 13
    };

    enum class ERHIPipelineStateType : uint8
    {
        Graphics = 0,
        Compute = 1
    };

    /** The maximum number of vertex elements which can be used by a vertex declaration. */
    enum
    {
        MaxVertexElementCount = 17,
        MaxVertexElementCount_NumBits = 5,
    };
    static_assert(MaxVertexElementCount <= (1 << MaxVertexElementCount_NumBits), "MaxVertexElementCount will not fit on MaxVertexElementCount_NumBits");

    struct RHITextureRegion
    {
        uint32 DstX;
        uint32 DstY;
        uint32 DstZ;
        uint32 SrcX;
        uint32 SrcY;
        uint32 SrcZ;
        uint32 Width;
        uint32 Height;
        uint32 DepthOrArraySize;
    };

    struct RHIRect
    {
        int32 Left;
        int32 Top;
        int32 Right;
        int32 Bottom;
    };
}

