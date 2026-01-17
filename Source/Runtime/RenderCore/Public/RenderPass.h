#pragma once
#include "Templates/RefCountObject.h"


namespace Thunder
{
    class RenderPass : RefCountedObject
    {
    public:
        RenderPass() = default;
        ~RenderPass() = default;

    protected:
        uint16 GlobalIndex = 0;
    };
}
