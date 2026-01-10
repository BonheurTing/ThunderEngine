#pragma once
#include "CoreMinimal.h"
#include "CommonUtilities.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
    struct PSOKey
    {
        union Value
        {
            uint64 Hash[2] = { 0, 0 };
            struct
            {
                uint32 CombinationIndex = 0;
                uint16 VertexDeclarationIndex = 0;
                uint16 RenderStateIndex = 0;
                
            };
        };
    };

    
}

