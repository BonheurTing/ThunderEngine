#include "ShaderModule.h"
#pragma optimize("",off)
#include "Assertion.h"
#include "CommonUtilities.h"
#include "FileHelper.h"
#include "rapidjson/document.h"

using namespace rapidjson;

namespace
{
	bool TryGetArray(Document& document, const char* arrayName, GenericArray<false, GenericValue<UTF8<>>> outArrayNode, const char* documentName, bool force = false)
	{
		if (document.HasMember(arrayName) && !document[arrayName].IsNull())
		{
			TAssertf(document[arrayName].IsArray(), "Shader meta parse error : Properties node is not an array\nFile name : %s.", documentName);
			outArrayNode = document[arrayName].GetArray();
			return true;
		}
		return false;
	}
	
	bool TryGetString(GenericValue<UTF8<>> const& object, const char* memberName, String& outValue, bool force = true, const char* defaultValue = "")
	{
		TAssertf(object.IsObject(), "Node is not an object.");

		// If member exists, just read it.
		if (object.HasMember(memberName))
		{
			if (object[memberName].IsString()) // String found.
			{
				outValue = object[memberName].GetString();
				return true;
			}
			else
			{
				outValue = defaultValue;
				TAssertf(false, "Non-string member \"%s\" found.", memberName);
				return false;
			}
		}
		else // If member not exists.
		{
			outValue = defaultValue; // If member not exists, set it to default value.
			if (force) // If this member is not optional, debug break here.
			{
				TAssertf(false, "Object has no member named \"%s\".", memberName);
				return false;
			}
			else
			{
				return true;
			}
		}
	}

	bool ParsePropertyNode(GenericValue<UTF8<>> const& object, ShaderPropertyMeta& outProperty)
	{
		TAssertf(object.IsObject(), "Property node is not an object.");
		bool result = true;
		result = result && TryGetString(object, "Name", outProperty.Name);
		result = result && TryGetString(object, "DisplayName", outProperty.DisplayName, false, outProperty.Name.c_str());
		result = result && TryGetString(object, "Type", outProperty.Type);
		result = result && TryGetString(object, "Default", outProperty.Default, false);
		result = result && TryGetString(object, "Range", outProperty.Range, false);
		return result;
	}

	bool ParseParameterNode(GenericValue<UTF8<>> const& object, ShaderParameterMeta& outParameter)
	{
		TAssertf(object.IsObject(), "Parameter node is not an object.");
		bool result = true;
		result = result && TryGetString(object, "Name", outParameter.Name);
		result = result && TryGetString(object, "Type", outParameter.Type);
		result = result && TryGetString(object, "Default", outParameter.Default, false);
		return result;
	}
	
	bool ParseRenderStateNode(GenericValue<UTF8<>> const& object, RenderStateMeta& outRenderState)
	{
		TAssertf(object.IsObject(), "RenderState node is not an object.");
		bool result = true;
		result = result && TryGetString(object, "Blend", outRenderState.Blend, false, "Opaque");
		result = result && TryGetString(object, "Cull", outRenderState.Cull, false, "Front");
		String lod;
		result = result && TryGetString(object, "LOD", lod);
		outRenderState.LOD = CommonUtilities::StringToInteger(lod, 1);
		return result;
	}

	bool ParseVariantNode(GenericValue<UTF8<>> const& object, VariantMeta& outVariant)
	{
		TAssertf(object.IsObject(), "Variant node is not an object.");
		bool result = true;
		result = result && TryGetString(object, "Name", outVariant.Name);
		result = result && TryGetString(object, "Texture", outVariant.Texture, false, "");
		String defaultValue, fallback, visible;
		result = result && TryGetString(object, "Default", defaultValue, false);
		return result;
	}

	bool ParseStageNode(GenericValue<UTF8<>> const& object, StageMeta& outStageMata, Array<VariantMeta>& outStageVariantMeta)
	{
		bool result = true;
		result = result && TryGetString(object, "EntryPoint", outStageMata.EntryPoint, true);
		// EntryVariants
		auto stageNode = object.GetObject();
		if (stageNode.HasMember("StageVariants") && !stageNode["StageVariants"].IsNull())
		{
			TAssertf(stageNode["StageVariants"].IsArray(), "Shader meta parse error : Stage EntryVariants node is not an array\n.");
			auto const& stageVariantsNode = stageNode["StageVariants"].GetArray();
			for (auto itr = stageVariantsNode.Begin(); itr != stageVariantsNode.End(); ++itr)
			{
				VariantMeta meta{};
				result = result && ParseVariantNode(*itr, meta);
				outStageVariantMeta.push_back(meta);
			}
		}
		return result;
	}
}


uint64 Pass::VariantNameToMask(Array<NameHandle>& definitionTable, Array<NameHandle>& variantName)
{
	return 1;
}

void Pass::VariantIdToVariantName(uint64 VariantId, Array<NameHandle>& definitionTable, Array<NameHandle>& variantName)
{
}

void Pass::GenerateVariantDefinitionTable()
{
	Set<NameHandle> variantDefinition;
	for (const auto& meta : PassVariantConfig)
	{
		variantDefinition.emplace(meta.Name);
	}
	for (auto [fst, snd] : StageVariantConfig)
	{
		for (const auto& meta : snd)
		{
			variantDefinition.emplace(meta.Name);
		}
	}
	VariantDefinitionTable.assign(variantDefinition.begin(), variantDefinition.end());
	//todo: gen PassVariantMask and StageVariantMask
	PassVariantMask = 0;
	//todo: gen default mask and default combination
}

bool ShaderModule::ParseShaderFile()
{
	Array<String> shaderNameList;
	FileHelper::TraverseFileFromFolderWithFormat(FileHelper::GetEngineRoot() + "\\Shader", shaderNameList, "shader");
	
	for (const String& metaFileName: shaderNameList)
	{
		String metaContent;
		LOG("%s\n", metaFileName.c_str());
		if (!FileHelper::LoadFileToString(metaFileName, metaContent))
		{
			continue;
		}
		Document document;
		document.Parse(metaContent.c_str());
		const ParseErrorCode errorCode = document.GetParseError();
		TAssertf(document.IsObject(), "Parse json file error.\nError code: %d\nFile name: \"%s\"", errorCode, metaFileName.c_str());
		
		TAssertf(document.HasMember("ShaderName"), "Shader name not found in %s", metaFileName.c_str());
		NameHandle shaderArchiveName = document["ShaderName"].GetString();
		TAssertf(document.HasMember("ShaderSource"), "Shader source path not found in %s", metaFileName.c_str());
		const String shaderArchiveSource = document["ShaderSource"].GetString();

		auto currentShader = new ShaderArchive(shaderArchiveSource);
		
		if (document.HasMember("Properties") && !document["Properties"].IsNull())
		{
			TAssertf(document["Properties"].IsArray(), "Shader meta parse error : Properties node is not an array\nFile name : %s.", metaFileName.c_str());
			auto const& shaderPropertiesNode = document["Properties"].GetArray();
			for (auto itr = shaderPropertiesNode.Begin(); itr != shaderPropertiesNode.End(); ++itr)
			{
				ShaderPropertyMeta meta{};
				ParsePropertyNode(*itr, meta);
				currentShader->AddPropertyMeta(meta);
			}
		}

		if (document.HasMember("ShaderParameters") && !document["ShaderParameters"].IsNull())
		{
			TAssertf(document["ShaderParameters"].IsArray(), "Shader meta parse error : ShaderParameters node is not an array\nFile name : %s.", metaFileName.c_str());
			auto const& shaderParametersNode = document["ShaderParameters"].GetArray();
			for (auto itr = shaderParametersNode.Begin(); itr != shaderParametersNode.End(); ++itr)
			{
				ShaderParameterMeta meta{};
				ParseParameterNode(*itr, meta);
				currentShader->AddParameterMeta(meta);
			}
		}

		if (document.HasMember("RenderState") && !document["RenderState"].IsNull())
		{
			TAssertf(document["RenderState"].IsObject(), "Shader meta parse error : RenderState node is not an object\nFile name : %s.", metaFileName.c_str());
			RenderStateMeta meta{};
			ParseRenderStateNode(document["RenderState"], meta);
			currentShader->SetRenderState(meta);
		}

		if (document.HasMember("Passes") && !document["Passes"].IsNull())
		{
			TAssertf(document["Passes"].IsObject(), "Shader meta parse error : Passes node is not an object\nFile name : %s.", metaFileName.c_str());
			auto shaderPasses = document["Passes"].GetObject();
			for (Value::ConstMemberIterator passItr = shaderPasses.MemberBegin(); passItr != shaderPasses.MemberEnd(); ++passItr)
			{
				const NameHandle passName = passItr->name.GetString();
				auto shaderPass = shaderPasses[passName.c_str()].GetObject();
				Pass* currentPass = new Pass(passName);
				// PassVariants
				if (shaderPass.HasMember("PassVariants") && !shaderPass["PassVariants"].IsNull())
				{
					TAssertf(shaderPass["PassVariants"].IsArray(), "Shader meta parse error : %s PassVariants node is not an array\nFile name : %s.", passName, metaFileName.c_str());
					auto const& passVariantsNode = shaderPass["PassVariants"].GetArray();
					Array<VariantMeta> PassVariantMeta;
					for (auto itr = passVariantsNode.Begin(); itr != passVariantsNode.End(); ++itr)
					{
						VariantMeta meta{};
						ParseVariantNode(*itr, meta);
						PassVariantMeta.push_back(meta);
					}
					currentPass->SetPassVariantMeta(PassVariantMeta);
				}
				// Vertex
				if (shaderPass.HasMember("Vertex") && !shaderPass["Vertex"].IsNull())
				{
					TAssertf(shaderPass["Vertex"].IsObject(), "Shader meta parse error : %s Vertex node is not an object\nFile name : %s.", passName, metaFileName.c_str());
					StageMeta sMeta{};
					Array<VariantMeta> StageVariantMeta;
					ParseStageNode(shaderPass["Vertex"], sMeta, StageVariantMeta);
					currentPass->AddStageMeta(EShaderStageType::Vertex, sMeta);
					if (!StageVariantMeta.empty())
					{
						currentPass->AddStageVariantMeta(EShaderStageType::Vertex, StageVariantMeta);
					}
				}
				// Pixel
				if (shaderPass.HasMember("Pixel") && !shaderPass["Pixel"].IsNull())
				{
					TAssertf(shaderPass["Pixel"].IsObject(), "Shader meta parse error : %s Pixel node is not an object\nFile name : %s.", passName, metaFileName.c_str());
					StageMeta sMeta{};
					Array<VariantMeta> StageVariantMeta;
					ParseStageNode(shaderPass["Pixel"], sMeta, StageVariantMeta);
					currentPass->AddStageMeta(EShaderStageType::Pixel, sMeta);
					if (!StageVariantMeta.empty())
					{
						currentPass->AddStageVariantMeta(EShaderStageType::Pixel, StageVariantMeta);
					}
				}
				currentPass->GenerateVariantDefinitionTable();
				currentShader->AddPass(passName, currentPass);
			}
		}

		ShaderCache.emplace(shaderArchiveName, currentShader);
		LOG("Succeed to Parse %s\n", metaFileName.c_str());
	}
	if(!shaderNameList.empty())
	{
		return true;
	}
	return false;
}
