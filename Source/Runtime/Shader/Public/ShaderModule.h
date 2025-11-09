#pragma once
#include "ShaderDefinition.h"
#include "Module/ModuleManager.h"
#include "ShaderCompiler.h"

namespace Thunder
{
	class ICompiler;
	class ShaderArchive;
    class SHADER_API ShaderModule : public IModule
    {
    	DECLARE_MODULE(Shader, ShaderModule)
    public:
    	void StartUp() override {}
    	void ShutDown() override;
    	void InitShaderCompiler(EGfxApiType type);
    	static void InitShaderMap();
    	
    	
    	ShaderArchive* GetShaderArchive(NameHandle name);
    	bool GetPassRegisterCounts(NameHandle shaderName, NameHandle passName, TShaderRegisterCounts& outRegisterCounts);
    	ShaderCombination* GetShaderCombination(const String& combinationHandle);
    	ShaderCombination* GetShaderCombination(NameHandle shaderName, NameHandle passName, uint64 variantId);

    	bool ParseShaderFile();
		void Compile(NameHandle archiveName, const String& inSource, const THashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, const THashMap<NameHandle, bool>& variantParameters, bool force = false);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 variantId, bool force = false);
    	
    private:
    	THashMap<NameHandle, ShaderArchive*> ShaderMap;
    	TRefCountPtr<ICompiler> ShaderCompiler;
    };
    
    extern SHADER_API THashMap<EShaderStageType, String> GShaderModuleTarget;
}
