
#include "RenderTranslator.h"

namespace Thunder
{
    ERHIVertexInputSemantic RenderTranslator::GetVertexSemanticFromName(const String& name)
    {
        String littleName = name;
        std::ranges::transform(littleName, littleName.begin(), ::tolower);
        if (littleName.find("position") == 0)
        {
            return ERHIVertexInputSemantic::Position;
        }
        else if (littleName.find("texcoord") == 0 || littleName.find("uv") == 0)
        {
            return ERHIVertexInputSemantic::TexCoord;
        }
        else if (littleName.find("normal") == 0)
        {
            return ERHIVertexInputSemantic::Normal;
        }
        else if (littleName.find("tangent") == 0)
        {
            return ERHIVertexInputSemantic::Tangent;
        }
        else if (littleName.find("color") == 0)
        {
            return ERHIVertexInputSemantic::Color;
        }
        else if (littleName.find("binormal") == 0)
        {
            return ERHIVertexInputSemantic::BiNormal;
        }
        else if (littleName.find("blendindices") == 0)
        {
            return ERHIVertexInputSemantic::BlendIndices;
        }
        else if (littleName.find("blendweights") == 0)
        {
            return ERHIVertexInputSemantic::BlendWeights;
        }

        return ERHIVertexInputSemantic::Unknown;
    }

    RHIFormat RenderTranslator::GetRHIFormatFromTypeKind(ETypeKind kind)
    {
        switch (kind)
        {
        case ETypeKind::Float:
            return RHIFormat::R32_FLOAT;
        case ETypeKind::Vector2f:
            return RHIFormat::R32G32_FLOAT;
        case ETypeKind::Vector3f:
            return RHIFormat::R32G32B32_FLOAT;
        case ETypeKind::Vector4f:
            return RHIFormat::R32G32B32A32_FLOAT;
        case ETypeKind::Int32:
            return RHIFormat::R32_SINT;
        case ETypeKind::UInt32:
            return RHIFormat::R32_UINT;
        case ETypeKind::Vector2i:
            return RHIFormat::R32G32_SINT;
        case ETypeKind::Vector3i:
            return RHIFormat::R32G32B32_SINT;
        case ETypeKind::Vector4i:
            return RHIFormat::R32G32B32A32_SINT;
        case ETypeKind::Vector2u:
            return RHIFormat::R32G32_UINT;
        case ETypeKind::Vector3u:
            return RHIFormat::R32G32B32_UINT;
        case ETypeKind::Vector4u:
            return RHIFormat::R32G32B32A32_UINT;
        case ETypeKind::Int16:
            return RHIFormat::R16_SINT;
        case ETypeKind::UInt16:
            return RHIFormat::R16_UINT;
        case ETypeKind::Int8:
            return RHIFormat::R8_SINT;
        case ETypeKind::UInt8:
            return RHIFormat::R8_UINT;
        default:
            return RHIFormat::UNKNOWN;
        }
    }
}
