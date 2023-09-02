#pragma once
#include "ShaderArchive.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
    class SHADER_API ShaderModule : public IModule
    {
		static ModuleRegistry<ShaderModule> ShaderRegistry;
    	//DEFINE_MODULE(Shader, ShaderModule)
    public:
    	static ShaderModule* GetInstance()
    	{
    		static auto inst = new ShaderModule();
    		return inst;
    	}
    	bool ParseShaderFile();
    	ShaderArchive* GetShaderArchive(NameHandle name);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, const HashMap<NameHandle, bool>& variantParameters, bool force = false);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 variantId, bool force = false);
    	
    private:
    	ShaderModule() = default;
    	HashMap<NameHandle, ShaderArchive*> ShaderMap;
    };
    
    extern SHADER_API HashMap<EShaderStageType, String> GShaderModuleTarget;
}
