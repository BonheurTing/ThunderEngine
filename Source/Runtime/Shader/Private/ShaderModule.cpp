#include "ShaderModule.h"
#include "Assertion.h"
#include "CoreModule.h"
//#pragma optimize("",off)
#include "FileManager.h"
#include "ShaderCompiler.h"
#include "rapidjson/document.h"
#include "ShaderArchive.h"
#include "Memory/MemoryBase.h"

namespace Thunder
{
	IMPLEMENT_MODULE(Shader, ShaderModule)
	
	using namespace rapidjson;
	namespace
	{
		
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
    		result = result && TryGetString(object, "Format", outProperty.Format, strstr(outProperty.Type.c_str(), "Texture"));
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
    		result = result && TryGetString(object, "Format", outParameter.Format, strstr(outParameter.Type.c_str(), "Texture"));
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

	ShaderModule::~ShaderModule()
	{
	}

	void ShaderModule::ShutDown()
	{
		for (auto pair : ShaderMap)
		{
			if (pair.second)
			{
				TMemory::Destroy(pair.second);
			}
		}
	}

	bool ShaderModule::ParseShaderFile()
    {
    	Array<String> shaderNameList;
    	GFileManager->TraverseFileFromFolderWithFormat(GFileManager->GetEngineRoot() + "\\Shader", shaderNameList, "shader");
    	
    	for (const String& metaFileName: shaderNameList)
    	{
    		String metaContent;
    		LOG("%s", metaFileName.c_str());
    		if (!GFileManager->LoadFileToString(metaFileName, metaContent))
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
    
    		auto currentShader = new (TMemory::Malloc<ShaderArchive>()) ShaderArchive {shaderArchiveSource, shaderArchiveName};
    		
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

    		if (document.HasMember("Parameters") && !document["Parameters"].IsNull())
    		{
    			TAssertf(document["Parameters"].IsArray(), "Shader meta parse error : Parameters node is not an array\nFile name : %s.", metaFileName.c_str());
    			auto const& parametersNode = document["Parameters"].GetArray();
    			for (auto itr = parametersNode.Begin(); itr != parametersNode.End(); ++itr)
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
    				ShaderPass* currentPass = new ShaderPass(passName);
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
    				// PassParameters
    				if (shaderPass.HasMember("PassParameters") && !shaderPass["PassParameters"].IsNull())
    				{
    					Array<ShaderParameterMeta> passParameterMeta;
    					TAssertf(shaderPass["PassParameters"].IsArray(), "Shader meta parse error : %s PassParameters node is not an array\nFile name : %s.", passName, metaFileName.c_str());
    					auto const& parametersNode = shaderPass["PassParameters"].GetArray();
    					for (auto itr = parametersNode.Begin(); itr != parametersNode.End(); ++itr)
    					{
    						ShaderParameterMeta meta{};
    						ParseParameterNode(*itr, meta);
    						passParameterMeta.push_back(meta);
    					}
    					currentShader->AddPassParameterMeta(passName, passParameterMeta);
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
    				TShaderRegisterCounts counts{};
    				currentShader->CalcRegisterCounts(passName, counts);
    				currentPass->SetShaderRegisterCounts(counts);
    				currentPass->CacheDefaultShaderCache();
    				currentShader->AddPass(passName, currentPass);
    			}
    		}
    
    		ShaderMap[shaderArchiveName] = currentShader;
    		LOG("Succeed to Parse %s", metaFileName.c_str());
    	}
    	if(!shaderNameList.empty())
    	{
    		return true;
    	}
    	return false;
    }

	ShaderArchive* ShaderModule::GetShaderArchive(NameHandle name)
	{
		if (ShaderMap.contains(name))
		{
		    return ShaderMap[name];
		}
		TAssertf(false, "ShaderArchive not exist");
		return nullptr;
	}
	
	void ShaderModule::InitShaderCompiler(EGfxApiType type)
	{
		switch (type)
		{
		case EGfxApiType::D3D12:
		{
			ShaderCompiler = MakeRefCount<FXCCompiler>();
			break;
		}
		case EGfxApiType::D3D11:
		{
			ShaderCompiler = MakeRefCount<FXCCompiler>();
			break;
		}
		case EGfxApiType::Invalid: break;
		}
	}
	
	void ShaderModule::Compile(NameHandle archiveName, const String& inSource, const HashMap<NameHandle, bool>& marco,
		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode)
	{
		ShaderCompiler->Compile(archiveName, inSource, inSource.size(), marco, includeStr, pEntryPoint, pTarget, outByteCode);
	}
	
    bool ShaderModule::CompileShaderCollection(NameHandle shaderType, NameHandle passName, const HashMap<NameHandle, bool>& variantParameters, bool force)
    {
    	return false;
    }
    
    bool ShaderModule::CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 VariantId, bool force)
    {
    	if (!ShaderMap.contains(shaderType))
    	{
    		return false;
    	}
    	return ShaderMap[shaderType]->CompileShaderPass(passName, VariantId);
    }
}

