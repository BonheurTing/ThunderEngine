#pragma once
#include "ShaderDefinition.h"
#include "Module/ModuleManager.h"
#include "ShaderCompiler.h"
#include "ShaderLang.h"
#include "RenderStates.h"
#include "ShaderBindingsLayout.h"
#include "Concurrent/Lock.h"

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

	    static bool GetPassRegisterCounts(ShaderArchive* archive, EMeshPass meshPassType, TShaderRegisterCounts& outRegisterCounts);
    	ShaderCombination* GetShaderCombination(const String& combinationHandle);
    	ShaderCombination* GetShaderCombination(NameHandle shaderName, NameHandle passName, uint64 variantId);

    	static void GenerateDefaultParameters(NameHandle archiveName, struct ShaderParameterMap* shaderParameterMap);
		void Compile(NameHandle archiveName, const String& inSource, const THashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, const THashMap<NameHandle, bool>& variantParameters, bool force = false);
    	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 variantId, bool force = false);
	    static ShaderCombination* CompileShaderVariant(NameHandle archiveName, NameHandle passName, uint64 variantId);
    	static void CompileShaderSource(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug = false);

    	// Translators.
	    static enum_shader_stage GetShaderASTStage(EShaderStageType stage);
	    static EShaderStageType GetShaderStage(enum_shader_stage stage);
	    static EMeshPass GetMeshPass(String const& passName);
	    static String GetMeshPassName(EMeshPass passType);
    	static enum_blend_mode GetShaderASTBlendMode(EShaderBlendMode blendMode);
    	static EShaderBlendMode GetBlendMode(enum_blend_mode blendMode);
    	static enum_depth_test GetShaderASTDepthState(EShaderDepthState depthTest);
    	static EShaderDepthState GetDepthState(enum_depth_test depthTest);

    	// Uniform Buffer Layout
    	void SetGlobalUniformBufferLayout(UniformBufferLayout* layout);
    	void SetPassUniformBufferLayout(EMeshPass pass, ShaderArchive* archive);
    	void SetPrimitiveUniformBufferLayout(UniformBufferLayout* layout);
    	static const UniformBufferLayout* GetGlobalUniformBufferLayout() { return GetModule()->GlobalUniformBufferLayout.Get(); }
    	static const UniformBufferLayout* GetPassUniformBufferLayout(EMeshPass pass);
    	static const UniformBufferLayout* GetPrimitiveUniformBufferLayout() { return GetModule()->PrimitiveUniformBufferLayout.Get(); }
    	static ShaderParameterMap* GetPassDefaultParameters(EMeshPass pass);

    private:
    	THashMap<NameHandle, ShaderArchive*> ShaderMap;
    	TRefCountPtr<ICompiler> ShaderCompiler;
    	THashMap<uint64, TRHIPipelineState*> PSOMap;

    	// uniform buffer manager
    	UniformBufferLayoutRef GlobalUniformBufferLayout;
    	TMap<EMeshPass, UniformBufferLayoutRef> PassUniformBufferLayoutMap;
    	TMap<EMeshPass, ShaderParameterMap*> PassDefaultParameterMap;
    	UniformBufferLayoutRef PrimitiveUniformBufferLayout;
    };
    
    extern SHADER_API THashMap<EShaderStageType, String> GShaderModuleTarget;
}
