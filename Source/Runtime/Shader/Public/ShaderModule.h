#pragma once
#include "Assertion.h"
#include "CommonUtilities.h"
#include "CoreMinimal.h"

enum class EShaderStageType : uint8
{
	Unknown	= 0,
	Vertex,
	Hull,
	Domain,
	Pixel,
	Compute,
	Mesh
};

struct VariantMeta
{
	String Name;
	bool Default; // if VariantType==Bool, True is 1, False is 0
	bool Fallback;
	bool Visible;
	String Texture;

	VariantMeta() {Default = false; Fallback = false; Visible = false;}
	VariantMeta(String name, const String& defaultValue = "", const String& fallback = "",
				const String& visible = "", String texture = "") : Name(std::move(name)), Texture(std::move(texture))
	{
		Default = CommonUtilities::StringToBool(defaultValue, false);
		Fallback = CommonUtilities::StringToBool(fallback, false);
		Visible = CommonUtilities::StringToBool(visible, true);
	}
};

struct ShaderStage
{
	BinaryData ByteCode;
	uint64 VariantId;
};

struct ShaderCombination
{
	HashMap<EShaderStageType, ShaderStage> Shaders;
};

struct StageMeta
{
	String EntryPoint;
	uint64 VariantMask;
};

class Pass
{
public:
	Pass() = delete;
	Pass(NameHandle name) : Name(name), PassVariantMask(0) {}
	static uint64 VariantNameToMask(Array<NameHandle>& definitionTable, Array<NameHandle>& variantName);
	static void VariantIdToVariantName(uint64 VariantId, Array<NameHandle>& definitionTable, Array<NameHandle>& variantName);
	void SetPassVariantMeta(const Array<VariantMeta>& meta) {PassVariantConfig = meta;}
	void AddStageVariantMeta(EShaderStageType type, const Array<VariantMeta>& meta) {StageVariantConfig.emplace(type, meta);}
	void AddStageMeta(EShaderStageType type, StageMeta& meta) {StageMetas.emplace(type, meta);}
	void GenerateVariantDefinitionTable();
private:
	NameHandle Name;
	Array<NameHandle> VariantDefinitionTable; // 改了之后相关递归mask都要改
	Array<VariantMeta> PassVariantConfig; // 考虑是临时的还是存着
	HashMap<EShaderStageType, Array<VariantMeta>> StageVariantConfig;// 考虑是临时的还是存着
	HashMap<EShaderStageType, StageMeta> StageMetas;
	uint64 PassVariantMask;
	HashMap<uint64, ShaderCombination> Variants;
};

struct ShaderPropertyMeta
{
	String Name;
	String DisplayName;
	String Type;
	String Default;
	String Range;
};

struct ShaderParameterMeta
{
	String Name;
	String Type;
	String Default;
};

struct RenderStateMeta
{
	String Blend;
	String Cull;
	int LOD;
};

class ShaderArchive
{
public:
	ShaderArchive() = delete;
	ShaderArchive(String inShaderSource) : SourcePath(std::move(inShaderSource)) {}

	void AddPass(NameHandle name, Pass* inPass)
	{
		Passes.emplace(name, inPass);
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
private:
	String SourcePath;
	Array<ShaderPropertyMeta> PropertyMeta;
	Array<ShaderParameterMeta> ParameterMeta;
	RenderStateMeta renderState {};
	HashMap<NameHandle, Pass*> Passes;
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
	
private:
	ShaderModule() = default;
	HashMap<NameHandle, ShaderArchive*> ShaderCache;
};
