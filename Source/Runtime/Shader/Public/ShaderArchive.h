#pragma once
#include "ShaderDefinition.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
	enum class EMeshPass : uint8;
	class ast_node;

	class ShaderPass : public RefCountedObject
    {
    public:
    	ShaderPass() = delete;
    	ShaderPass(NameHandle name) : Name(name), PassVariantMask(0) {}
		void SetShaderRegisterCounts(const TShaderRegisterCounts& counts) { RegisterCounts = counts; }
        _NODISCARD_ TShaderRegisterCounts GetShaderRegisterCounts() const { return RegisterCounts; }
    	uint64 VariantNameToMask(const TArray<ShaderVariantMeta>& variantName) const;
    	void VariantIdToShaderMarco(uint64 variantId, uint64 variantMask, THashMap<NameHandle, bool>& shaderMarco) const;
    	//void SetPassVariantMeta(const Array<VariantMeta>& meta) {VariantDefinitionTable = meta;}
    	void AddStageMeta(EShaderStageType type, const StageMeta& meta) {StageMetas[type] = meta;}
    	void GenerateVariantDefinitionTable(const TArray<ShaderVariantMeta>& passVariantMeta, const THashMap<EShaderStageType, TArray<ShaderVariantMeta>>& stageVariantMeta);
    	void CacheDefaultShaderCache();
    	bool CheckCache(uint64 variantId) const
    	{
    		return Variants.contains(variantId);
    	}
		ShaderCombination* GetShaderCombination(uint64 variantId)
    	{
    		if (CheckCache(variantId))
    		{
    			return Variants[variantId].Get();
    		}
    		return nullptr;
    	}
    	bool CompileShader(NameHandle archiveName, const String& shaderSource, const String& includeStr, uint64 variantId);
		bool RegisterRootSignature()
		{
			//TD3D12RHIModule::GetRootSignatureManager().RegisterRootSignature(Name, RegisterCounts);
			return false;
		}
    private:
    	NameHandle Name;
    	uint64 PassVariantMask;
		TShaderRegisterCounts RegisterCounts{};
    	TArray<ShaderVariantMeta> VariantDefinitionTable; // 改了之后相关递归mask都要改
    	THashMap<EShaderStageType, StageMeta> StageMetas;
    	THashMap<uint64, ShaderCombinationRef> Variants;
    };
	using ShaderPassRef = TRefCountPtr<ShaderPass>;

	struct ShaderAST
	{
		ShaderAST(ast_node* inRoot): ASTRoot(inRoot) {}
		~ShaderAST();
		ast_node* ASTRoot = nullptr;

		// parse ast to obtain Property/Variant/Parameters
		void ParseAllTypeParameters(class ShaderArchive* archive) const;
	};

    class ShaderArchive
    {
    public:
    	ShaderArchive() = delete;
    	ShaderArchive(String inShaderSource, NameHandle name) : SourcePath(std::move(inShaderSource)), Name(name) {}
    	ShaderArchive(String sourceFilePath, NameHandle shaderName, ast_node* astRoot);
    	~ShaderArchive();
    
    	void AddPass(NameHandle name, ShaderPass* inPass)
    	{
    		Passes.emplace(name, ShaderPassRef(inPass));
    	}
    	void AddPropertyMeta(const ShaderPropertyMeta& meta)
    	{
    		PropertyMeta.push_back(meta);
    	}
    	void AddVariantMeta(const ShaderVariantMeta& meta)
    	{
    		VariantMeta.push_back(meta);
    	}
    	void AddGlobalParameterMeta(const ShaderParameterMeta& meta)
    	{
    		GlobalParameterMeta.push_back(meta);
    	}
    	void AddObjectParameterMeta(const ShaderParameterMeta& meta)
    	{
    		ObjectParameterMeta.push_back(meta);
    	}
    	void AddPassParameterMeta(NameHandle name, const TArray<ShaderParameterMeta>& metas)
    	{
    		PasseParameterMeta.emplace(name, metas);
    	}
    	void SetRenderState(const RenderStateMeta& meta) {renderState = meta;}
    
    	ShaderPass* GetPass(NameHandle name);
    	ShaderPass* GetSubShader(EMeshPass meshPassType);
    	String GetShaderSourceDir() const;
    	void GenerateIncludeString(NameHandle passName, String& outFile);
		void CalcRegisterCounts(NameHandle passName, TShaderRegisterCounts& outCount);
    	bool CompileShaderPass(NameHandle passName, uint64 variantId, bool force = false);
    	
    private:
    	String SourcePath;
    	NameHandle Name;
    	ShaderAST* AST { nullptr }; // ShaderAST manages ast root lifetime

    	// all type parameters from ast
    	TArray<ShaderPropertyMeta> PropertyMeta; // visible
    	TArray<ShaderVariantMeta> VariantMeta; // partially visible
    	TArray<ShaderParameterMeta> GlobalParameterMeta; // global parameters
    	TArray<ShaderParameterMeta> ObjectParameterMeta; // object parameters
    	THashMap<NameHandle, TArray<ShaderParameterMeta>> PasseParameterMeta; //Based on the usage within the subshader

    	RenderStateMeta renderState {};
    	THashMap<NameHandle, ShaderPassRef> Passes; //deprecated
    	THashMap<EMeshPass, ShaderPassRef> SubShaders;  // = Passes
    };
    
}
