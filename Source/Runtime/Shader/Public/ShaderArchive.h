#pragma once
#include "ShaderDefinition.h"
#include "Concurrent/Lock.h"
#include "Templates/RefCounting.h"
#include "AstNode.h"
#include "MeshPass.h"
#include "RenderStates.h"
#include "ShaderBindingsLayout.h"

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
    	ShaderPass(class ShaderArchive* archive, const ast_node_pass* astNode);
		void SetShaderRegisterCounts(const TShaderRegisterCounts& counts) { RegisterCounts = counts; }
        _NODISCARD_ TShaderRegisterCounts GetShaderRegisterCounts() const { return RegisterCounts; }
    	void AddStageMeta(EShaderStageType type, const StageMeta& meta) {StageMetas[type] = meta;}
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

		_NODISCARD_ ShaderArchive* GetArchive() const { return Archive; }
		_NODISCARD_ NameHandle GetName() const { return Name; }
		_NODISCARD_ EMeshPass GetMeshPass() const { return MeshPass; }
		_NODISCARD_ bool IsMeshPass() const { return static_cast<uint32>(MeshPass) < static_cast<uint32>(EMeshPass::Num); }
		
		EShaderBlendMode GetBlendMode() const { return BlendMode; }
		EShaderDepthState GetDepthState() const { return DepthState; }
		EShaderStencilState GetStencilState() const { return StencilState; }

    private:
		ShaderArchive* Archive = nullptr;
    	NameHandle Name;
		TShaderRegisterCounts RegisterCounts{};
    	THashMap<EShaderStageType, StageMeta> StageMetas;
		SharedLock VariantsLock;
    	THashMap<uint64, ShaderCombinationRef> Variants;
		SharedLock SyncCompilingVariantsLock;
    	THashMap<uint64, SyncCompilingCombinationEntry*> SyncCompilingVariants;

		// Pass meta.
		EMeshPass MeshPass = EMeshPass::Num;
		EShaderBlendMode BlendMode = EShaderBlendMode::Unknown;
		EShaderDepthState DepthState = EShaderDepthState::Unknown;
		EShaderStencilState StencilState = EShaderStencilState::Unknown;
    };
	using ShaderPassRef = TRefCountPtr<ShaderPass>;

	struct ShaderAST
	{
		ShaderAST(ast_node* inRoot): ASTRoot(inRoot) {}
		~ShaderAST();

		// parse ast to obtain Property/Variant/Parameters
		void Reflect(class ShaderArchive* archive) const;
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

    	void AddSubShader(ShaderPass* inSubShader);
    	void AddPropertyMeta(const ShaderPropertyMeta& meta)
    	{
    		PropertyMeta.push_back(meta);
    	}
    	void AddVariantMeta(const ShaderVariantMeta& meta)
    	{
    		uint64 currentVariantCount = VariantMeta.size();
    		TAssertf(currentVariantCount <= 32, "Variant count can't be greater than 32, archive name \"%s\".", GetName().c_str());
    		if (currentVariantCount < 32)
    		{
    			VariantMeta.push_back(meta);
    		}
    	}
    	FORCEINLINE TArray<ShaderParameterMeta>&  EnsureAndGetUniformBufferMetaList(String const& uniformBufferName)
    	{
    		return UniformParameterMeta[uniformBufferName];
    	}
    	FORCEINLINE TArray<ShaderParameterMeta>& GetUniformBufferMetaList(String const& uniformBufferName)
    	{
    		TAssertf(UniformParameterMeta.contains(uniformBufferName), "Uniform buffer \"%s\" does not exist.", uniformBufferName.c_str());
    		return UniformParameterMeta[uniformBufferName];
    	}
    	void AddUniformBufferParameterMeta(String const& uniformBufferName, ShaderParameterMeta const& meta)
    	{
    		UniformParameterMeta[uniformBufferName].push_back(meta);
    	}
    	void SetRenderState(const RenderStateMeta& meta) { RenderState = meta; }

	    _NODISCARD_ NameHandle GetName() const { return Name; }
    	_NODISCARD_ String GetSourcePath() const { return SourcePath; }
    	ShaderPass* GetSubShader(NameHandle name);
    	ShaderPass* GetSubShader(EMeshPass meshPassType);
    	String GetShaderSourceDir() const;
		void CalcRegisterCounts(NameHandle passName, TShaderRegisterCounts& outCount);
	    static void QuantizeRegisterCounts(TShaderRegisterCounts& outCount);
    	ShaderCombination* CompileShaderVariant(NameHandle subShaderName, uint64 variantId);
    	ShaderAST* GetAST() const { return AST; }

    	String GenerateShaderSource(ShaderCodeGenConfig const& config) const;
    	String GetSubShaderEntry(String const& subShaderName, EShaderStageType stageType) const;
    	MaterialParameterCache GenerateParameterCache();
    	void ParseVariants(ShaderCodeGenConfig const& config, shader_codegen_state& state) const;
    	uint64 VariantNameToMask(const TMap<NameHandle, bool>& variantMap) const;
    	void VariantMaskToName(uint64 variantMask, THashMap<NameHandle, bool>& variantMap) const;
    	void BuildUniformBufferLayout();
    	void BuildBindingsLayout();
    	ShaderBindingsLayout* GetBindingsLayout() const { return BindingsLayout; }
    	UniformBufferLayout* GetUniformBufferLayout(String const& cbName) { return UniformBufferLayoutMap[cbName]; }

    private:
    	friend class ShaderAST;
    	String SourcePath;
    	NameHandle Name;
    	ShaderAST* AST { nullptr }; // ShaderAST manages ast root lifetime

    	// all type parameters from ast
    	TArray<ShaderPropertyMeta> PropertyMeta; // visible
    	TArray<ShaderVariantMeta> VariantMeta; // partially visible
    	TMap<String, TArray<ShaderParameterMeta>> UniformParameterMeta;

    	RenderStateMeta RenderState {};
    	THashMap<NameHandle, ShaderPassRef> SubShaders;
    	THashMap<EMeshPass, ShaderPassRef> MeshDrawSubShaders;

    	ShaderBindingsLayoutRef BindingsLayout{ nullptr };
    	TMap<String, UniformBufferLayoutRef> UniformBufferLayoutMap;
    };
    
}
