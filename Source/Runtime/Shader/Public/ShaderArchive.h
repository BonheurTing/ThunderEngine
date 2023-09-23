#pragma once
#include "ShaderDefinition.h"

namespace Thunder
{
	class ShaderPass
    {
    public:
    	ShaderPass() = delete;
    	ShaderPass(NameHandle name) : Name(name), PassVariantMask(0) {}
		void SetShaderRegisterCounts(const TShaderRegisterCounts& counts)
    	{
    		RegisterCounts = MakeRefCount<TShaderRegisterCounts>(counts);
    	}
    	uint64 VariantNameToMask(const Array<VariantMeta>& variantName) const;
    	void VariantIdToShaderMarco(uint64 variantId, uint64 variantMask, HashMap<NameHandle, bool>& shaderMarco) const;
    	//void SetPassVariantMeta(const Array<VariantMeta>& meta) {VariantDefinitionTable = meta;}
    	void AddStageMeta(EShaderStageType type, const StageMeta& meta) {StageMetas[type] = meta;}
    	void GenerateVariantDefinitionTable(const Array<VariantMeta>& passVariantMeta, const HashMap<EShaderStageType, Array<VariantMeta>>& stageVariantMeta);
    	void CacheDefaultShaderCache();
    	bool CheckCache(uint64 variantId) const
    	{
    		return Variants.contains(variantId);
    	}
    	void UpdateOrAddVariants(uint64 variantId, ShaderCombination& variant) {Variants[variantId] = std::move(variant);}
    	bool CompileShader(NameHandle archiveName, const String& shaderSource, const String& includeStr, uint64 variantId);
		bool RegisterRootSignature()
		{
			//todo: get rootsignaturemanager
			return false;
		}
    private:
    	NameHandle Name;
    	uint64 PassVariantMask;
		RefCountPtr<TShaderRegisterCounts> RegisterCounts;
    	Array<VariantMeta> VariantDefinitionTable; // 改了之后相关递归mask都要改
    	HashMap<EShaderStageType, StageMeta> StageMetas;
    	HashMap<uint64, ShaderCombination> Variants;
    };
    
    class ShaderArchive
    {
    public:
    	ShaderArchive() = delete;
    	ShaderArchive(String inShaderSource, NameHandle name) : Name(name), SourcePath(std::move(inShaderSource)) {}
    
    	void AddPass(NameHandle name, ShaderPass* inPass)
    	{
    		Passes.emplace(name, RefCountPtr<ShaderPass>(inPass));
    	}
    	void AddPropertyMeta(const ShaderPropertyMeta& meta)
    	{
    		PropertyMeta.push_back(meta);
    	}
    	void AddParameterMeta(const ShaderParameterMeta& meta)
    	{
    		ParameterMeta.push_back(meta);
    	}
    	void AddPassParameterMeta(NameHandle name, const Array<ShaderParameterMeta>& metas)
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
    	Array<ShaderPropertyMeta> PropertyMeta;
    	Array<ShaderParameterMeta> ParameterMeta;
    	HashMap<NameHandle, Array<ShaderParameterMeta>> PasseParameterMeta;
    	RenderStateMeta renderState {};
    	HashMap<NameHandle, RefCountPtr<ShaderPass>> Passes;
    };
    
}
