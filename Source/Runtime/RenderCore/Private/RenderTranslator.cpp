
#include "RenderTranslator.h"

namespace Thunder
{
    ERHIVertexInputSemantic RenderTranslator::GetVertexSemanticFromName(const String& name)
    {
        if (name.find("Position") != String::npos || name.find("position") != String::npos)
        {
            return ERHIVertexInputSemantic::Position;
        }
        else if (name.find("TexCoord") != String::npos || name.find("UV") != String::npos || 
                 name.find("texcoord") != String::npos || name.find("uv") != String::npos)
        {
            return ERHIVertexInputSemantic::TexCoord;
        }
        else if (name.find("Normal") != String::npos || name.find("normal") != String::npos)
        {
            return ERHIVertexInputSemantic::Normal;
        }
        else if (name.find("Tangent") != String::npos || name.find("tangent") != String::npos)
        {
            return ERHIVertexInputSemantic::Tangent;
        }
        else if (name.find("Color") != String::npos || name.find("color") != String::npos)
        {
            return ERHIVertexInputSemantic::Color;
        }
        else if (name.find("BiNormal") != String::npos || name.find("binormal") != String::npos)
        {
            return ERHIVertexInputSemantic::BiNormal;
        }
        else if (name.find("BlendIndices") != String::npos || name.find("blendindices") != String::npos)
        {
            return ERHIVertexInputSemantic::BlendIndices;
        }
        else if (name.find("BlendWeights") != String::npos || name.find("blendweights") != String::npos)
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
