#pragma once
#include "ShaderDefinition.h"

class Pass
{
public:
	Pass() = delete;
	Pass(NameHandle name) : Name(name), PassVariantMask(0) {}
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
	bool CompileShader(const String& shaderSource, uint64 variantId);
private:
	NameHandle Name;
	uint64 PassVariantMask;
	Array<VariantMeta> VariantDefinitionTable; // 改了之后相关递归mask都要改
	HashMap<EShaderStageType, StageMeta> StageMetas;
	HashMap<uint64, ShaderCombination> Variants;
};

class ShaderArchive
{
public:
	ShaderArchive() = delete;
	ShaderArchive(String inShaderSource) : SourcePath(std::move(inShaderSource)) {}

	void AddPass(NameHandle name, Pass* inPass)
	{
		Passes.emplace(name, RefCountPtr<Pass>(inPass));
	}
	void AddPropertyMeta(const ShaderPropertyMeta& meta)
	{
		PropertyMeta.push_back(meta);
	}
	void AddParameterMeta(const ShaderParameterMeta& meta)
	{
		ParameterMeta.push_back(meta);
	}
	void SetRenderState(const RenderStateMeta& meta) {renderState = meta;}

	Pass* GetPass(NameHandle name);

	bool CompileShaderPass(NameHandle passName, uint64 variantId, bool force = false);
private:
	String SourcePath;
	Array<ShaderPropertyMeta> PropertyMeta;
	Array<ShaderParameterMeta> ParameterMeta;
	RenderStateMeta renderState {};
	HashMap<NameHandle, RefCountPtr<Pass>> Passes;
};

class SHADER_API ShaderModule
{
public:
	static ShaderModule* GetInstance()
	{
		static auto inst = new ShaderModule();
		return inst;
	}
	bool ParseShaderFile();
	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, const HashMap<NameHandle, bool>& variantParameters, bool force = false);
	bool CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 variantId, bool force = false);
	
private:
	ShaderModule() = default;
	HashMap<NameHandle, ShaderArchive*> ShaderCache;
};
