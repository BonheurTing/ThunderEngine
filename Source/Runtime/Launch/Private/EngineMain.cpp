#include "EngineMain.h"
#include "D3D12RHI.h"
#include "D3D11RHI.h"
#include "ShaderCompiler.h"

namespace Thunder
{
    bool EngineMain::RHIInit(ERHIType type)
    {
        switch (type)
        {
            case ERHIType::D3D12:
            {
                GDynamicRHI = new D3D12DynamicRHI();
                GShaderCompiler = new FXCCompiler();
                return GDynamicRHI != nullptr ? true : false;
            }
            case ERHIType::D3D11:
            {
                GDynamicRHI = new D3D11DynamicRHI();
                GShaderCompiler = new FXCCompiler();
                return GDynamicRHI != nullptr ? true : false;
            }
            default:
                return false;
        }
    }
}

