#pragma once
#include "ShaderDefinition.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
	class ShaderPass : public RefCountedObject
    {
    public:
    	ShaderPass() = delete;
    	ShaderPass(NameHandle name) : Name(name), PassVariantMask(0) {}
		void SetShaderRegisterCounts(const TShaderRegisterCounts& counts) { RegisterCounts = counts; }
        _NODISCARD_ TShaderRegisterCounts GetShaderRegisterCounts() const { return RegisterCounts; }
    	uint64 VariantNameToMask(const TArray<VariantMeta>& variantName) const;
    	void VariantIdToShaderMarco(uint64 variantId, uint64 variantMask, THashMap<NameHandle, bool>& shaderMarco) const;
    	//void SetPassVariantMeta(const Array<VariantMeta>& meta) {VariantDefinitionTable = meta;}
    	void AddStageMeta(EShaderStageType type, const StageMeta& meta) {StageMetas[type] = meta;}
    	void GenerateVariantDefinitionTable(const TArray<VariantMeta>& passVariantMeta, const THashMap<EShaderStageType, TArray<VariantMeta>>& stageVariantMeta);
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
    	TArray<VariantMeta> VariantDefinitionTable; // 改了之后相关递归mask都要改
    	THashMap<EShaderStageType, StageMeta> StageMetas;
    	THashMap<uint64, TRefCountPtr<ShaderCombination>> Variants;
    };
    
    class ShaderArchive
    {
    public:
    	ShaderArchive() = delete;
    	ShaderArchive(String inShaderSource, NameHandle name) : Name(name), SourcePath(std::move(inShaderSource)) {}
    
    	void AddPass(NameHandle name, ShaderPass* inPass)
    	{
    		Passes.emplace(name, TRefCountPtr<ShaderPass>(inPass));
    	}
    	void AddPropertyMeta(const ShaderPropertyMeta& meta)
    	{
    		PropertyMeta.push_back(meta);
    	}
    	void AddParameterMeta(const ShaderParameterMeta& meta)
    	{
    		ParameterMeta.push_back(meta);
    	}
    	void AddPassParameterMeta(NameHandle name, const TArray<ShaderParameterMeta>& metas)
    	{
    		PasseParameterMeta.emplace(name, metas);
    	}
    	void SetRenderState(const RenderStateMeta& meta) {renderState = meta;}
    
    	ShaderPass* GetPass(NameHandle name);
    	String GetShaderSourceDir() const;
    	void GenerateIncludeString(NameHandle passName, String& outFile);
		void CalcRegisterCounts(NameHandle passName, TShaderRegisterCounts& outCount);
    	bool CompileShaderPass(NameHandle passName, uint64 variantId, bool force = false);
    	
    private:
    	NameHandle Name;
    	String SourcePath;
    	TArray<ShaderPropertyMeta> PropertyMeta;
    	TArray<ShaderParameterMeta> ParameterMeta;
    	THashMap<NameHandle, TArray<ShaderParameterMeta>> PasseParameterMeta;
    	RenderStateMeta renderState {};
    	THashMap<NameHandle, TRefCountPtr<ShaderPass>> Passes;
    };
    
}
