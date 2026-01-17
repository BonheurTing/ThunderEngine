#pragma once
#include "Platform.h"

namespace Thunder
{
    enum class EShaderBlendMode : uint8
    {
        Unknown = 0,
        Opaque,
        Translucent
    };

    enum class EShaderDepthState : uint8
    {
        Unknown = 0,
        Less,
        Greater,
        Equal,
        Never,
        Always
    };

    enum class EShaderStencilState : uint8
    {
        Unknown = 0,
        Default
    };
}
