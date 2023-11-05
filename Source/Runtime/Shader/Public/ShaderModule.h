#pragma once
#include "ShaderDefinition.h"
#include "Module/ModuleManager.h"

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
    	bool ParseShaderFile();
    	ShaderArchive* GetShaderArchive(NameHandle name);
    	bool GetPassRegisterCounts(NameHandle shaderName, NameHandle passName, TShaderRegisterCounts& outRegisterCounts);
    	bool GetShaderCombination(NameHandle shaderName, NameHandle passName, uint64 variantId, ShaderCombination*& outShaderCombination);
    	void InitShaderCompiler(EGfxApiType type);
		void Compile(NameHandle archiveName, const String& inSource, const HashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, const HashMap<NameHandle, bool>& variantParameters, bool force = false);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 variantId, bool force = false);
    	
    private:
    	HashMap<NameHandle, ShaderArchive*> ShaderMap;
    	RefCountPtr<ICompiler> ShaderCompiler;
    };
    
    extern SHADER_API HashMap<EShaderStageType, String> GShaderModuleTarget;
}
