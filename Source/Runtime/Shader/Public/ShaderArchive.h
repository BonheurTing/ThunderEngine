#pragma once
#include "ShaderDefinition.h"
#include "../../Renderer/Public/MeshPassProcessor.h"
#include "Concurrent/Lock.h"
#include "Templates/RefCounting.h"

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

	class ShaderPass : public RefCountedObject
    {
    public:
    	ShaderPass() = delete;
    	ShaderPass(class ShaderArchive* archive, NameHandle name) : Archive(archive), Name(name) {}
		void SetShaderRegisterCounts(const TShaderRegisterCounts& counts) { RegisterCounts = counts; }
        _NODISCARD_ TShaderRegisterCounts GetShaderRegisterCounts() const { return RegisterCounts; }
    	//uint64 VariantNameToMask(const TArray<ShaderVariantMeta>& variantName) const;
		uint64 VariantNameToMask(const TMap<NameHandle, bool>& parameters) const;
    	void VariantIdToShaderMarco(uint64 variantId, uint64 variantMask, THashMap<NameHandle, bool>& shaderMarco) const;
    	void AddStageMeta(EShaderStageType type, const StageMeta& meta) {StageMetas[type] = meta;}
		//void SetPassVariantMeta(const Array<VariantMeta>& meta) {VariantDefinitionTable = meta;}
		void SetVariantDefinitionTable(const TArray<ShaderVariantMeta>& passVariantMeta);
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
    	uint64 PassVariantMask = 0;
		TShaderRegisterCounts RegisterCounts{};
    	TArray<ShaderVariantMeta> VariantDefinitionTable;
    	THashMap<EShaderStageType, StageMeta> StageMetas; // Deprecated.
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
		String GenerateShaderVariantSource(NameHandle passName, const TArray<ShaderVariantMeta>& variants);
		String GenerateShaderVariantSource(NameHandle passName, uint64 variantId);
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
    		MeshDrawSubShaders.emplace(EMeshPass::BasePass, ShaderPassRef(inSubShader)); // LdwTodo : Get mesh-draw type.
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

	    _NODISCARD_ NameHandle GetName() const { return Name; }
    	_NODISCARD_ String GetSourcePath() const { return SourcePath; }
    	ShaderPass* GetSubShader(NameHandle name);
    	ShaderPass* GetSubShader(EMeshPass meshPassType);
    	String GetShaderSourceDir() const;
    	void GenerateIncludeString(NameHandle passName, String& outFile);
		void CalcRegisterCounts(NameHandle passName, TShaderRegisterCounts& outCount);
    	ShaderCombination* CompileShaderVariant(NameHandle subShaderName, uint64 variantId);
    	ShaderAST* GetAST() const { return AST; }

    	String GenerateShaderSource(NameHandle passName, uint64 variantId);
    	String GetSubShaderEntry(String const& subShaderName, EShaderStageType stageType) const;
    	MaterialParameterCache GenerateParameterCache();
    	
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
