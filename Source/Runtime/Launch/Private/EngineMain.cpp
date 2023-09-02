#include "EngineMain.h"
#include "D3D12RHI.h"
#include "D3D11RHI.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"

namespace Thunder
{
    bool EngineMain::RHIInit(EGfxApiType type)
    {
        ShaderModule::GetInstance()->InitShaderCompiler(type);
        switch (type)
        {
            case EGfxApiType::D3D12:
            {
                GDynamicRHI = new D3D12DynamicRHI();
                return GDynamicRHI != nullptr ? true : false;
            }
            case EGfxApiType::D3D11:
            {
                GDynamicRHI = new D3D11DynamicRHI();
                return GDynamicRHI != nullptr ? true : false;
            }
            case EGfxApiType::Invalid: return false;
        }
        return false;
    }
}

