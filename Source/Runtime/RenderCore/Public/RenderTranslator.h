#pragma once

#include "MathUtilities.h"
#include "MeshPass.h"
#include "RHIDefinition.h"
#include "ShaderDefinition.h"
#include "ShaderLang.h"
#include "Container/ReflectiveContainer.h"

namespace Thunder
{
    class RenderTranslator
    {
    public:
        static ERHIVertexInputSemantic GetVertexSemanticFromName(const String& name);
        static RHIFormat GetRHIFormatFromTypeKind(ETypeKind kind);
    };
}
