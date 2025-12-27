#pragma once
#include "ShaderDefinition.h"
#include "Module/ModuleManager.h"
#include "ShaderCompiler.h"
#include "ShaderLang.h"

namespace Thunder
{
	class TRHIPipelineState;
	enum class EMeshPass : uint8;
	class ICompiler;
	class ShaderArchive;

	enum EFixedVariant : uint64
	{
		EFixedVariant_ShaderVariantsBegin = 0,
		EFixedVariant_GlobalVariantsBegin = 32,

		EFixedVariant_GlobalVariant0 = 1ull << (EFixedVariant_GlobalVariantsBegin + 0),
		EFixedVariant_GlobalVariant1 = 1ull << (EFixedVariant_GlobalVariantsBegin + 1),
	};
	extern const THashMap<EFixedVariant, String> GFixedVariantMap;
	
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
	    static ShaderCombination* CompileShaderVariant(NameHandle archiveName, NameHandle passName, uint64 variantId);
    	static void CompileShaderSource(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug = false);
	    static enum_shader_stage GetShaderASTStage(EShaderStageType stage);
	    static EShaderStageType GetShaderStage(enum_shader_stage stage);

    private:
    	static ShaderCombination* SyncCompileShaderCombination(ShaderArchive* archive, NameHandle passName, uint64 variantMask);
    	
    private:
    	THashMap<NameHandle, ShaderArchive*> ShaderMap;
    	TRefCountPtr<ICompiler> ShaderCompiler;
    	THashMap<uint64, TRHIPipelineState*> PSOMap;
    };
    
    extern SHADER_API THashMap<EShaderStageType, String> GShaderModuleTarget;
}
