#pragma once
#include "ShaderDefinition.h"
#include "../../Renderer/Public/MeshPassProcessor.h"
#include "Concurrent/Lock.h"
#include "Templates/RefCounting.h"
#include "AstNode.h"

namespace Thunder
{
	enum class EMeshPass : uint8;
	class ast_node;

	struct SyncCompilingCombinationEntry
	{
		ShaderCombination* Combination = nullptr;
		SharedLock Lock{};

		SyncCompilingCombinationEntry() = default;
		~SyncCompilingCombinationEntry() = default;
		SyncCompilingCombinationEntry(const SyncCompilingCombinationEntry& rhs) : Combination(rhs.Combination) {}
		SyncCompilingCombinationEntry& operator=(const SyncCompilingCombinationEntry& rhs)
		{
			if (this != &rhs)
			{
				Combination = rhs.Combination;
			}
			return *this;
		}
	};

	struct ShaderCodeGenConfig
	{
		NameHandle SubShaderName;
		uint64 VariantMask;
		EShaderStageType Stage;
	};

	class ShaderPass : public RefCountedObject
    {
    public:
    	ShaderPass() = delete;
    	ShaderPass(class ShaderArchive* archive, NameHandle name) : Archive(archive), Name(name) {}
		void SetShaderRegisterCounts(const TShaderRegisterCounts& counts) { RegisterCounts = counts; }
        _NODISCARD_ TShaderRegisterCounts GetShaderRegisterCounts() const { return RegisterCounts; }
    	void AddStageMeta(EShaderStageType type, const StageMeta& meta) {StageMetas[type] = meta;}
    	void GenerateVariantDefinitionTable_Deprecated(const TArray<ShaderVariantMeta>& passVariantMeta, const THashMap<EShaderStageType, TArray<ShaderVariantMeta>>& stageVariantMeta);
    	void CacheDefaultShaderCache();

    	FORCEINLINE bool CheckCache(uint64 variantId)
    	{
    		auto lock = VariantsLock.Read();
    		return Variants.contains(variantId);
    	}

		FORCEINLINE ShaderCombination* GetShaderCombination(uint64 variantId)
    	{
    		auto lock = VariantsLock.Read();
    		auto variantIt = Variants.find(variantId);
    		if (variantIt != Variants.end())
    		{
    			return variantIt->second.Get();
    		}
    		return nullptr;
    	}

		ShaderCombination* GetOrCompileShaderCombination(uint64 variantId);

    	bool CompileShader(NameHandle archiveName, const String& shaderSource, const String& includeStr, uint64 variantId);
    	ShaderCombination* CompileShaderVariant(uint64 variantId);
		bool RegisterRootSignature()
		{
			//TD3D12RHIModule::GetRootSignatureManager().RegisterRootSignature(Name, RegisterCounts);
			return false;
		}

		_NODISCARD_ NameHandle GetName() const { return Name; }
		
    private:
		ShaderArchive* Archive = nullptr;
    	NameHandle Name;
		TShaderRegisterCounts RegisterCounts{};
    	THashMap<EShaderStageType, StageMeta> StageMetas;
		SharedLock VariantsLock;
    	THashMap<uint64, ShaderCombinationRef> Variants;
		SharedLock SyncCompilingVariantsLock;
    	THashMap<uint64, SyncCompilingCombinationEntry*> SyncCompilingVariants;
    };
	using ShaderPassRef = TRefCountPtr<ShaderPass>;

	struct ShaderAST
	{
		ShaderAST(ast_node* inRoot): ASTRoot(inRoot) {}
		~ShaderAST();

		// parse ast to obtain Property/Variant/Parameters
		void ParseAllTypeParameters(class ShaderArchive* archive) const;
    	String GenerateShaderVariantSource(shader_codegen_state& state) const;
		String GetSubShaderEntry(String const& subShaderName, EShaderStageType stageType) const;

	private:
		ast_node* ASTRoot = nullptr;
	};

    class ShaderArchive
    {
    public:
    	ShaderArchive() = delete;
    	ShaderArchive(String inShaderSource, NameHandle name) : SourcePath(std::move(inShaderSource)), Name(name) {}
    	ShaderArchive(String sourceFilePath, NameHandle shaderName, ast_node* astRoot);
    	~ShaderArchive();

    	void AddSubShader(ShaderPass* inSubShader)
    	{
    		SubShaders.emplace(inSubShader->GetName(), ShaderPassRef(inSubShader));
    		MeshDrawSubShaders.emplace(EMeshPass::BasePass, ShaderPassRef(inSubShader)); // 12.28Todo : Get mesh-draw type.
    	}
    	void AddPropertyMeta(const ShaderPropertyMeta& meta)
    	{
    		PropertyMeta.push_back(meta);
    	}
    	void AddVariantMeta(const ShaderVariantMeta& meta)
    	{
    		uint64 currentVariantCount = VariantMeta.size();
    		TAssert(currentVariantCount <= 32, "Variant count can't be greater than 32, archive name \"%s\".", GetName().c_str());
    		if (currentVariantCount < 32)
    		{
    			VariantMeta.push_back(meta);
    		}
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

	    _NODISCARD_ NameHandle GetName() const { return Name; }
    	_NODISCARD_ String GetSourcePath() const { return SourcePath; }
    	ShaderPass* GetSubShader(NameHandle name);
    	ShaderPass* GetSubShader(EMeshPass meshPassType);
    	String GetShaderSourceDir() const;
    	void GenerateIncludeString(NameHandle passName, String& outFile);
		void CalcRegisterCounts(NameHandle passName, TShaderRegisterCounts& outCount);
    	ShaderCombination* CompileShaderVariant(NameHandle subShaderName, uint64 variantId);
    	ShaderAST* GetAST() const { return AST; }

    	String GenerateShaderSource(ShaderCodeGenConfig const& config) const;
    	String GetSubShaderEntry(String const& subShaderName, EShaderStageType stageType) const;
    	MaterialParameterCache GenerateParameterCache();
    	void ParseVariants(ShaderCodeGenConfig const& config, shader_codegen_state& state) const;
    	uint64 VariantNameToMask(const TMap<NameHandle, bool>& variantMap) const;
    	void VariantMaskToName(uint64 variantMask, THashMap<NameHandle, bool>& variantMap) const;

    private:
    	friend class ShaderAST;
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
    	THashMap<NameHandle, ShaderPassRef> SubShaders;
    	THashMap<EMeshPass, ShaderPassRef> MeshDrawSubShaders;
    };
    
}
