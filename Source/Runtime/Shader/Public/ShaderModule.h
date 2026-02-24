#pragma once
#include "ShaderDefinition.h"
#include "Module/ModuleManager.h"
#include "ShaderCompiler.h"
#include "ShaderLang.h"
#include "RenderStates.h"
#include "ShaderBindingsLayout.h"

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
	
    class ShaderModule : public IModule
    {
    	DECLARE_MODULE(Shader, ShaderModule, SHADER_API)

    public:
    	SHADER_API void StartUp() override {}
    	SHADER_API void ShutDown() override;
    	SHADER_API void InitShaderCompiler(EGfxApiType type);
    	SHADER_API static void InitShaderMap();

    	// Shader
    	SHADER_API static ShaderArchive* GetShaderArchive(NameHandle name);
    	SHADER_API static uint64 GetVariantMask(ShaderArchive* archive, const TMap<NameHandle, bool>& parameters);
    	SHADER_API static ShaderCombination* GetShaderCombination(ShaderArchive* archive, EMeshPass meshPassType, uint64 variantMask);
    	SHADER_API static ShaderCombination* GetShaderCombination(ShaderArchive* archive, NameHandle subShaderName, uint64 variantMask);
	    SHADER_API static bool GetPassRegisterCounts(ShaderArchive* archive, EMeshPass meshPassType, TShaderRegisterCounts& outRegisterCounts);
	    SHADER_API static bool GetPassRegisterCounts(ShaderArchive* archive, NameHandle subShaderName, TShaderRegisterCounts& outRegisterCounts);
		SHADER_API static TRHIPipelineState* GetPSO(uint64 psoKey);

    	SHADER_API static void GenerateDefaultParameters(NameHandle archiveName, struct ShaderParameterMap* shaderParameterMap);
		SHADER_API void Compile(NameHandle archiveName, const String& inSource, const THashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode);
    	SHADER_API bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, const THashMap<NameHandle, bool>& variantParameters, bool force = false);
    	SHADER_API bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 variantId, bool force = false);
	    SHADER_API static ShaderCombination* CompileShaderVariant(NameHandle archiveName, NameHandle passName, uint64 variantId);
    	SHADER_API static void CompileShaderSource(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug = false);

    	// Translators.
	    SHADER_API static enum_shader_stage GetShaderASTStage(EShaderStageType stage);
	    SHADER_API static EShaderStageType GetShaderStage(enum_shader_stage stage);
	    SHADER_API static EMeshPass GetMeshPass(String const& passName);
	    SHADER_API static String GetMeshPassName(EMeshPass passType);
    	SHADER_API static enum_blend_mode GetShaderASTBlendMode(EShaderBlendMode blendMode);
    	SHADER_API static EShaderBlendMode GetBlendMode(enum_blend_mode blendMode);
    	SHADER_API static enum_depth_test GetShaderASTDepthState(EShaderDepthState depthTest);
    	SHADER_API static EShaderDepthState GetDepthState(enum_depth_test depthTest);

    	// Uniform Buffer Layout
    	SHADER_API void SetGlobalUniformBufferLayout(UniformBufferLayout* layout);
    	SHADER_API void SetPassUniformBufferLayout(EMeshPass pass, ShaderArchive* archive);
    	SHADER_API void SetPrimitiveUniformBufferLayout(ShaderArchive* archive);
    	SHADER_API static const UniformBufferLayout* GetGlobalUniformBufferLayout() { return GetModule()->GlobalUniformBufferLayout.Get(); }
    	SHADER_API static const UniformBufferLayout* GetPassUniformBufferLayout(EMeshPass pass);
    	SHADER_API static const UniformBufferLayout* GetPrimitiveUniformBufferLayout() { return GetModule()->PrimitiveUniformBufferLayout.Get(); }
    	SHADER_API static ShaderParameterMap* GetPassDefaultParameters(EMeshPass pass);

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
