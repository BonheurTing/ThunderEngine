#include "ShaderModule.h"
#pragma optimize("",off)
#include "Assertion.h"
#include "CommonUtilities.h"
#include "FileHelper.h"
#include "ShaderCompiler.h"
#include "rapidjson/document.h"

//////////////////////////////////////////////////////////////////////////////////////////
/// Parse Shader File
//////////////////////////////////////////////////////////////////////////////////////////

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
		String name;
		result = result && TryGetString(object, "Name", name);
		outProperty.Name = name;
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
		String name;
		result = result && TryGetString(object, "Name", name);
		result = result && TryGetString(object, "Type", outParameter.Type);
		result = result && TryGetString(object, "Default", outParameter.Default, false);
		outParameter.Name = name;
		return result;
	}
	
	bool ParseRenderStateNode(GenericValue<UTF8<>> const& object, RenderStateMeta& outRenderState)
	{
		TAssertf(object.IsObject(), "RenderState node is not an object.");
		bool result = true;
		String lod;
		result = result && TryGetString(object, "LOD", lod);
		result = result && TryGetString(object, "Blend", outRenderState.Blend, false, "Opaque");
		result = result && TryGetString(object, "Cull", outRenderState.Cull, false, "Front");
		outRenderState.LOD = CommonUtilities::StringToInteger(lod, 1);
		return result;
	}

	bool ParseVariantNode(GenericValue<UTF8<>> const& object, VariantMeta& outVariant)
	{
		TAssertf(object.IsObject(), "Variant node is not an object.");
		bool result = true;
		String name, defaultValue, fallback, visible, texture;
		result = result && TryGetString(object, "Name", name);
		result = result && TryGetString(object, "Texture", texture, false, "");
		result = result && TryGetString(object, "Default", defaultValue, false);
		outVariant = {name, defaultValue, fallback, visible, texture};
		return result;
	}

	bool ParseStageNode(GenericValue<UTF8<>> const& object, StageMeta& outStageMata, Array<VariantMeta>& outStageVariantMeta)
	{
		TAssertf(object.IsObject(), "Shader meta parse error : Stage node is not an object\n");

		bool result = true;
		String entryPoint;
		result = result && TryGetString(object, "EntryPoint", entryPoint, true);
		outStageMata.EntryPoint = entryPoint;
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

uint64 Pass::VariantNameToMask(const Array<VariantMeta>& variantName) const
{
	if (!variantName.empty())
	{
		uint64 mask = 0;
		const int totalVariantType = static_cast<int>(VariantDefinitionTable.size());
		for (auto meta : variantName)
		{
			int i = 0;
			for (; i < totalVariantType; i++)
			{
				if (meta.Name == VariantDefinitionTable[i].Name)
				{
					mask = mask | 1 << i;
					break;
				}
			}
			TAssertf(i < totalVariantType, "Couldn't find variant %s", meta.Name);
		}
		return mask;
	}
	return 0;
}

void Pass::VariantIdToShaderMarco(uint64 variantId, uint64 variantMask, HashMap<NameHandle, bool>& shaderMarco) const
{
	const int totalVariantType = static_cast<int>(VariantDefinitionTable.size());
	TAssertf(variantId >> totalVariantType == 0, "Error input VariantId");
	for (int i = 0; i < totalVariantType; i++)
	{
		if (variantMask & 1 << i)
		{
			shaderMarco[VariantDefinitionTable[i].Name] = (variantId & 1 << i) > 0;
		}
	}
}

void Pass::GenerateVariantDefinitionTable(const Array<VariantMeta>& passVariantMeta, const HashMap<EShaderStageType, Array<VariantMeta>>& stageVariantMeta)
{
	// gen VariantDefinitionTable
	HashSet<NameHandle> variantDefinition;
	for (auto meta : passVariantMeta)
	{
		TAssertf(!variantDefinition.contains(meta.Name), "Duplicate variant definitions: %s", meta.Name);
		variantDefinition.insert(meta.Name);
		VariantDefinitionTable.push_back(meta);
	}

	for (auto [fst, snd] : stageVariantMeta)
	{
		for (const auto& meta : snd)
		{
			TAssertf(!variantDefinition.contains(meta.Name), "Duplicate variant definitions: %s", meta.Name);
			variantDefinition.insert(meta.Name);
			VariantDefinitionTable.push_back(meta);
		}
	}
	TAssertf(variantDefinition.size() == VariantDefinitionTable.size(), "Invalid variant definitions");
	
	// gen PassVariantMask and StageVariantMask
	PassVariantMask = 0;
	for (int i = 0; i < static_cast<int>(VariantDefinitionTable.size()); i++)
	{
		PassVariantMask = PassVariantMask << 1 | 1;
	}
	const uint64 sharedStageMask = VariantNameToMask(passVariantMeta);
	for (auto meta:  StageMetas)
	{
		meta.second.VariantMask = sharedStageMask;
		if (stageVariantMeta.contains(meta.first))
		{
			meta.second.VariantMask = meta.second.VariantMask | VariantNameToMask( stageVariantMeta.at(meta.first) );
		}
	}
}

void Pass::CacheDefaultShaderCache()
{
	//todo: gen default mask and default combination
	//todo: compile default
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
				Array<VariantMeta> passVariantMeta;
				if (shaderPass.HasMember("PassVariants") && !shaderPass["PassVariants"].IsNull())
				{
					TAssertf(shaderPass["PassVariants"].IsArray(), "Shader meta parse error : %s PassVariants node is not an array\nFile name : %s.", passName, metaFileName.c_str());
					auto const& passVariantsNode = shaderPass["PassVariants"].GetArray();
					for (auto itr = passVariantsNode.Begin(); itr != passVariantsNode.End(); ++itr)
					{
						VariantMeta meta{};
						ParseVariantNode(*itr, meta);
						passVariantMeta.push_back(meta);
					}
				}
				HashMap<EShaderStageType, Array<VariantMeta>> stageVariantConfig;
				// Vertex
				if (shaderPass.HasMember("Vertex") && !shaderPass["Vertex"].IsNull())
				{
					StageMeta sMeta{};
					Array<VariantMeta> StageVariantMeta;
					ParseStageNode(shaderPass["Vertex"], sMeta, StageVariantMeta);
					currentPass->AddStageMeta(EShaderStageType::Vertex, sMeta);
					if (!StageVariantMeta.empty())
					{
						stageVariantConfig[EShaderStageType::Vertex] = StageVariantMeta;
					}
				}
				// Pixel
				if (shaderPass.HasMember("Pixel") && !shaderPass["Pixel"].IsNull())
				{
					StageMeta sMeta{};
					Array<VariantMeta> StageVariantMeta;
					ParseStageNode(shaderPass["Pixel"], sMeta, StageVariantMeta);
					currentPass->AddStageMeta(EShaderStageType::Pixel, sMeta);
					if (!StageVariantMeta.empty())
					{
						stageVariantConfig[EShaderStageType::Pixel] = StageVariantMeta;
					}
				}
				// Compute
				if (shaderPass.HasMember("Compute") && !shaderPass["Compute"].IsNull())
				{
					StageMeta sMeta{};
					Array<VariantMeta> StageVariantMeta;
					ParseStageNode(shaderPass["Compute"], sMeta, StageVariantMeta);
					currentPass->AddStageMeta(EShaderStageType::Compute, sMeta);
					if (!StageVariantMeta.empty())
					{
						stageVariantConfig[EShaderStageType::Compute] = StageVariantMeta;
					}
				}
				currentPass->GenerateVariantDefinitionTable(passVariantMeta, stageVariantConfig);
				currentPass->CacheDefaultShaderCache();
				currentShader->AddPass(passName, currentPass);
			}
		}

		ShaderCache[shaderArchiveName] = currentShader;
		LOG("Succeed to Parse %s\n", metaFileName.c_str());
	}
	if(!shaderNameList.empty())
	{
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Compile Shader
//////////////////////////////////////////////////////////////////////////////////////////
bool Pass::CompileShader(const String& shaderSource, uint64 variantId)
{
	TAssertf((PassVariantMask & variantId) == variantId, "Compile Shader: Invalid variantId");
	// Stage
	ShaderCombination newVariant{};
	for (auto meta : StageMetas)
	{
		// Variant Marco
		TAssertf((meta.second.VariantMask & PassVariantMask) == meta.second.VariantMask, "Compile Shader: Invalid stage variantId");
		const uint64 stageVariantId = variantId & meta.second.VariantMask;
		HashMap<NameHandle, bool> shaderMarco{};
		VariantIdToShaderMarco(stageVariantId, meta.second.VariantMask, shaderMarco);
		//todo: include file
		ShaderStage newStageVariant{};
		Compile(shaderSource, shaderMarco, meta.second.EntryPoint.c_str(), GShaderModuleTarget[meta.first], newStageVariant.ByteCode);

		if(newStageVariant.ByteCode.Size == 0)
		{
			TAssertf(false, "Compile Shader: Output an empty ByteCode");
			return false;
		}
		newVariant.Shaders[meta.first] = newStageVariant;
	}
	UpdateOrAddVariants(variantId, newVariant);
	return true;
}

Pass* ShaderArchive::GetPass(NameHandle name)
{
	if (Passes.contains(name))
	{
		return Passes[name].get();
	}
	TAssertf(false, "Get null pass");
	return nullptr;
}

bool ShaderArchive::CompileShaderPass(NameHandle passName, uint64 variantId, bool force)
{
	Pass* targetPass = GetPass(passName);
	if (!force && targetPass && targetPass->CheckCache(variantId))
	{
		return true;
	}
	// SourcePath
	const String fileName = FileHelper::GetEngineRoot() + "\\Shader\\" + SourcePath;
	String shaderSource;
	FileHelper::LoadFileToString(fileName, shaderSource);
	return targetPass->CompileShader(shaderSource, variantId);
}

bool ShaderModule::CompileShaderCollection(NameHandle shaderType, NameHandle passName, const HashMap<NameHandle, bool>& variantParameters, bool force)
{
	return false;
}

bool ShaderModule::CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 VariantId, bool force)
{
	if (!ShaderCache.contains(shaderType))
	{
		return false;
	}
	return ShaderCache[shaderType]->CompileShaderPass(passName, VariantId);
}
