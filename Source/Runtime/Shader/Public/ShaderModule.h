#pragma once
#include "ShaderDefinition.h"
#include "Module/ModuleManager.h"
#include "ShaderCompiler.h"

namespace Thunder
{
	class TRHIPipelineState;
	enum class EMeshPass : uint8;
	class ICompiler;
	class ShaderArchive;

	enum FixedVariantMask : uint64
	{
		ShaderVariantsBegin = 0,
		VertexVariantsBegin = 32,
		GlobalVariantsBegin = 48,

		EnableVertexColor = 1 << VertexVariantsBegin,
		EnableUv2 = EnableVertexColor << 1,

		EnableLumen = 1 << GlobalVariantsBegin
	};
	
    class SHADER_API ShaderModule : public IModule
    {
    	DECLARE_MODULE(Shader, ShaderModule)
    public:
    	void StartUp() override {}
    	void ShutDown() override;
    	void InitShaderCompiler(EGfxApiType type);
    	static void InitShaderMap();
    	
    	static ShaderArchive* GetShaderArchive(NameHandle name);
    	static uint64 GetVariantMask(ShaderArchive* archive, const TMap<NameHandle, bool>& parameters);
    	static ShaderCombination* GetShaderCombination(ShaderArchive* archive, EMeshPass meshPassType, uint64 variantMask); // meshPassType -> subShaderName
		static TRHIPipelineState* GetPSO(uint64 psoKey);

    	bool GetPassRegisterCounts(NameHandle shaderName, NameHandle passName, TShaderRegisterCounts& outRegisterCounts);
    	ShaderCombination* GetShaderCombination(const String& combinationHandle);
    	ShaderCombination* GetShaderCombination(NameHandle shaderName, NameHandle passName, uint64 variantId);

    	bool ParseShaderFile_Deprecated();
    	static MaterialParameterCache ParseShaderParameters(NameHandle archiveName);
		void Compile(NameHandle archiveName, const String& inSource, const THashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, const THashMap<NameHandle, bool>& variantParameters, bool force = false);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 variantId, bool force = false);

    private:
    	static ShaderCombination* SyncCompileShaderCombination(ShaderArchive* archive, NameHandle passName, uint64 variantMask);
    	
    private:
    	THashMap<NameHandle, ShaderArchive*> ShaderMap;
    	TRefCountPtr<ICompiler> ShaderCompiler;
    	THashMap<uint64, TRHIPipelineState*> PSOMap;
    };
    
    extern SHADER_API THashMap<EShaderStageType, String> GShaderModuleTarget;
}
