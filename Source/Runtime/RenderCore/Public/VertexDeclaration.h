#pragma once

#include "MathUtilities.h"
#include "RenderResource.h"

namespace Thunder
{
    struct VertexDeclarationKey
    {
        uint64 Hash[2] = { 0, 0 };
    };

    class VertexDeclaration
    {
    public:
        VertexDeclaration() = default;
        ~VertexDeclaration() = default;

    protected:
        uint16 GlobalIndex = 0;
        VertexDeclarationKey GlobalKey;
    };
}
