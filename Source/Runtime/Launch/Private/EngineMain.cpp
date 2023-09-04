#include "EngineMain.h"
#include "D3D12RHIModule.h"
#include "D3D11RHIModule.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
    bool EngineMain::RHIInit(EGfxApiType type)
    {
        ShaderModule::GetInstance()->InitShaderCompiler(type);
        switch (type)
        {
            case EGfxApiType::D3D12:
            {
                TD3D12RHIModule::EnsureLoad();
                ModuleManager::GetInstance()->LoadModuleByName("D3D12RHI");
                break;
            }
            case EGfxApiType::D3D11:
            {
                TD3D11RHIModule::EnsureLoad();
                ModuleManager::GetInstance()->LoadModuleByName("D3D11RHI");
                break;
            }
            case EGfxApiType::Invalid: return false;
        }
        
        return true;
    }
}

