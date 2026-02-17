
#pragma optimize("", off)

#include "ShaderModule.h"
#include <fstream>
#include <sstream>
#include "Assertion.h"
#include "CoreModule.h"
#include "CRC.h"
#include "MeshPass.h"
#include "PreProcessor.h"
#include "ShaderLang.h"
#include "ShaderCompiler.h"
#include "rapidjson/document.h"
#include "ShaderArchive.h"
#include "ShaderParameterMap.h"
#include "Concurrent/TaskScheduler.h"
#include "FileSystem/FileModule.h"
#include "Memory/MemoryBase.h"

extern Thunder::shader_lang_state* ThunderParse(const char* text);
namespace Thunder
{
	IMPLEMENT_MODULE(Shader, ShaderModule)
	
	const THashMap<EFixedVariant, String> GFixedVariantMap = 
	{
		{ EFixedVariant_GlobalVariant0, "_GlobalVariant0" },
		{ EFixedVariant_GlobalVariant1, "_GlobalVariant1" },
	};

	void ShaderModule::ShutDown()
	{
		for (auto archive : ShaderMap | std::views::values)
		{
			if (archive)
			{
				TMemory::Destroy(archive);
			}
		}
		for (auto param : PassDefaultParameterMap | std::views::values)
		{
			if (param)
			{
				delete param;
			}
		}
	}

	void ShaderModule::InitShaderCompiler(EGfxApiType type)
	{
		switch (type)
		{
		case EGfxApiType::D3D12:
			{
				ShaderCompiler = MakeRefCount<DXCCompiler>();
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

	// load all shader archive
	
	void ShaderModule::InitShaderMap()
	{
		TArray<String> shaderNameList;
		FileModule::TraverseFileFromFolderWithFormat(FileModule::GetEngineShaderRoot(), shaderNameList, "tsf");
		
		for (const String& metaFileName: shaderNameList)
		{
			LOG("ParseShaderFile: %s", metaFileName.c_str());
			std::ifstream file(metaFileName);
			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string raw_text = buffer.str();
			String processed_text = PreProcessor::Process(raw_text).c_str();
			shader_lang_state* st = ThunderParse(processed_text.c_str());
			fflush(stdout);

			ShaderArchive* newArchive = new (TMemory::Malloc<ShaderArchive>()) ShaderArchive(metaFileName, st->shader_name, st->ast_root);
			GetModule()->ShaderMap[st->shader_name] = newArchive;
			LOG("Succeed to Parse %s", metaFileName.c_str());
		}

		// print all shader
		LOG("--------- All Shader %d Begin ---------", static_cast<int32>(GetModule()->ShaderMap.size()));
		for (const auto& [fst, snd] : GetModule()->ShaderMap)
		{
			LOG("Name: %s, SourcePath: %s", fst.c_str(), snd->GetSourcePath().c_str());
		}
		LOG("--------- All Shader End ---------\n");
	}

	uint64 ShaderModule::GetVariantMask(ShaderArchive* archive, const TMap<NameHandle, bool>& parameters)
	{
		// Global variants.
		uint64 variantMask = 0;
		if (GConfigManager->GetConfig("BaseEngine")->GetBool("EnableRenderFeature0"))
		{
			variantMask |= EFixedVariant_GlobalVariant0;
		}
		if (GConfigManager->GetConfig("BaseEngine")->GetBool("EnableRenderFeature1"))
		{
			variantMask |= EFixedVariant_GlobalVariant1;
		}

		// Shader variants.
		variantMask |= archive->VariantNameToMask(parameters);

		return variantMask;
	}

	ShaderCombination* ShaderModule::GetShaderCombination(ShaderArchive* archive, EMeshPass meshPassType, uint64 variantMask)
	{
		if (ShaderPass* subShader = archive->GetSubShader(meshPassType))
		{
			return subShader->GetOrCompileShaderCombination(variantMask);
		}
		return nullptr;
	}

	TRHIPipelineState* ShaderModule::GetPSO(uint64 psoKey)
	{
		if (GetModule()->PSOMap.contains(psoKey))
		{
			/*
			GAsyncWorkers->PushTask([psoKey]()
			{ 
			});
			*/
		}
		return GetModule()->PSOMap[psoKey];
	}

	void ShaderModule::GenerateDefaultParameters(NameHandle archiveName, ShaderParameterMap* shaderParameterMap)
	{
		if (ShaderArchive* archive = GetShaderArchive(archiveName))
		{
			archive->GenerateDefaultParameters(shaderParameterMap);
		}
	}

	ShaderArchive* ShaderModule::GetShaderArchive(NameHandle name)
	{
		if (GetModule()->ShaderMap.contains(name))
		{
		    return GetModule()->ShaderMap[name];
		}
		TAssertf(false, "ShaderArchive not exist");
		return nullptr;
	}

	bool ShaderModule::GetPassRegisterCounts(ShaderArchive* archive, EMeshPass meshPassType, TShaderRegisterCounts& outRegisterCounts)
	{
		if (ShaderPass* subShader = archive->GetSubShader(meshPassType))
		{
			outRegisterCounts = subShader->GetShaderRegisterCounts();
			return true;
		}
		TAssertf(false, "Sub-shader \"%s\" not exist in archive \"%s\"", ShaderModule::GetMeshPassName(meshPassType).c_str(), archive->GetName().c_str());
		return false;
	}

	ShaderCombination* ShaderModule::GetShaderCombination(const String& combinationHandle)
	{
		size_t pos1 = 0, pos2 = 0;
		pos1 = combinationHandle.find_first_of('|');
		TAssert(pos1 != String::npos);
		const NameHandle shaderName = combinationHandle.substr(0, pos1);
		pos2 = combinationHandle.find_last_of('|');
		TAssert(pos2 != String::npos && pos2 > pos1);
		const NameHandle passName = combinationHandle.substr(pos1 + 1, pos2 - pos1 - 1);
		const uint64 variantId = CommonUtilities::StringToInteger(combinationHandle.substr(pos2 + 1));
		return GetShaderCombination(shaderName, passName, variantId);
	}

	ShaderCombination* ShaderModule::GetShaderCombination(NameHandle shaderName, NameHandle passName, uint64 variantId)
	{
		if (!ShaderMap.contains(shaderName))
		{
			TAssertf(false, "ShaderArchive not exist");
			return nullptr;
		}
		if (const auto pass = ShaderMap[shaderName]->GetSubShader(passName))
		{
			if (pass->CheckCache(variantId))
			{
				return pass->GetShaderCombination(variantId);
			}
			else
			{
				//todo compile?
			}
		}
		TAssertf(false, "ShaderPass not exist");
		return nullptr;
	}
	
	void ShaderModule::Compile(NameHandle archiveName, const String& inSource, const THashMap<NameHandle, bool>& marco,
		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode)
	{
		ShaderCompiler->Compile(archiveName, inSource, inSource.size(), marco, includeStr, pEntryPoint, pTarget, outByteCode);
	}
	
    bool ShaderModule::CompileShaderCollection(NameHandle shaderType, NameHandle passName, const THashMap<NameHandle, bool>& variantParameters, bool force)
    {
    	return false;
    }
    
    bool ShaderModule::CompileShaderCollection(NameHandle shaderType, NameHandle passName, uint64 VariantId, bool force)
    {
    	if (!ShaderMap.contains(shaderType))
    	{
			TAssertf(false, "ShaderArchive not exist");
    		return false;
    	}
    	// return ShaderMap[shaderType]->CompileShaderVariant(passName, VariantId);
		return true;
    }

    ShaderCombination* ShaderModule::CompileShaderVariant(NameHandle archiveName, NameHandle passName, uint64 variantId)
	{
		auto& shaderMap = GetModule()->ShaderMap;
		auto archiveIt = shaderMap.find(archiveName);
		if (archiveIt == shaderMap.end()) [[unlikely]]
		{
			TAssertf(false, "Failed to compile shader : archive \"%s\" not exist.", archiveName.c_str());
			return nullptr;
		}
		ShaderArchive* archive = archiveIt->second;
		return archive->CompileShaderVariant(passName, variantId);
    }

    void ShaderModule::CompileShaderSource(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug)
	{
		GetModule()->ShaderCompiler->Compile(inSource, entryPoint, stage, outByteCode, debug);
    }

    enum_shader_stage ShaderModule::GetShaderASTStage(EShaderStageType stage)
    {
		switch (stage)
		{
		case EShaderStageType::Vertex:
			return enum_shader_stage::vertex;
		case EShaderStageType::Pixel:
			return enum_shader_stage::pixel;
		case EShaderStageType::Hull:
			return enum_shader_stage::hull;
		case EShaderStageType::Domain:
			return enum_shader_stage::domain;
		case EShaderStageType::Geometry:
			return enum_shader_stage::geometry;
		case EShaderStageType::Compute:
			return enum_shader_stage::compute;
		case EShaderStageType::Mesh:
			return enum_shader_stage::mesh;
		case EShaderStageType::Unknown:
		default:
			TAssertf(false, "Unexpected shader stage : %d.", static_cast<int>(stage));
			return enum_shader_stage::unknown;
		}
    }

    EShaderStageType ShaderModule::GetShaderStage(enum_shader_stage stage)
	{
		switch (stage)
		{
		case enum_shader_stage::vertex:
			return EShaderStageType::Vertex;
		case enum_shader_stage::pixel:
			return EShaderStageType::Pixel;
		case enum_shader_stage::hull:
			return EShaderStageType::Hull;
		case enum_shader_stage::domain:
			return EShaderStageType::Domain;
		case enum_shader_stage::geometry:
			return EShaderStageType::Geometry;
		case enum_shader_stage::compute:
			return EShaderStageType::Compute;
		case enum_shader_stage::mesh:
			return EShaderStageType::Mesh;
		case enum_shader_stage::max:
		case enum_shader_stage::unknown:
		default:
			TAssertf(false, "Unexpected shader stage : %d.", static_cast<int>(stage));
			return EShaderStageType::Unknown;
		}
    }

    EMeshPass ShaderModule::GetMeshPass(String const& passName)
    {
		if (passName == "PrePass")
		{
			return EMeshPass::PrePass;
		}
		else if (passName == "BasePass")
		{
			return EMeshPass::BasePass;
		}
		else if (passName == "ShadowPass")
		{
			return EMeshPass::ShadowPass;
		}
		else if (passName == "Translucent")
		{
			return EMeshPass::Translucent;
		}

		TAssertf(false, "Unknown mesh pass type : %s.", passName.c_str());
		return EMeshPass::Num;
    }

    String ShaderModule::GetMeshPassName(EMeshPass passType)
    {
		switch (passType)
		{
		case EMeshPass::PrePass:
			return "PrePass";
		case EMeshPass::BasePass:
			return "BasePass";
		case EMeshPass::ShadowPass:
			return "ShadowPass";
		case EMeshPass::Translucent:
			return "Translucent";
		case EMeshPass::Num:
		default:
			return "Unknown";
		}
    }

    enum_blend_mode ShaderModule::GetShaderASTBlendMode(EShaderBlendMode blendMode)
    {
	    switch (blendMode)
	    {
		case EShaderBlendMode::Opaque:
	    	return enum_blend_mode::opaque;
	    case EShaderBlendMode::Translucent:
	    	return enum_blend_mode::translucent;
	    default:
	    	return enum_blend_mode::undefined;
	    }
    }

    EShaderBlendMode ShaderModule::GetBlendMode(enum_blend_mode blendMode)
	{
		switch (blendMode)
		{
		case enum_blend_mode::opaque:
			return EShaderBlendMode::Opaque;
		case enum_blend_mode::translucent:
			return EShaderBlendMode::Translucent;
		default:
			return EShaderBlendMode::Unknown;
		}
    }

    enum_depth_test ShaderModule::GetShaderASTDepthState(EShaderDepthState depthTest)
    {
		switch (depthTest)
		{
		case EShaderDepthState::Never:
			return enum_depth_test::never;
		case EShaderDepthState::Less:
			return enum_depth_test::less;
		case EShaderDepthState::Greater:
			return enum_depth_test::greater;
		case EShaderDepthState::Equal:
			return enum_depth_test::equal;
		case EShaderDepthState::Always:
			return enum_depth_test::always;
		default:
			return enum_depth_test::undefined;
		}
    }

    EShaderDepthState ShaderModule::GetDepthState(enum_depth_test depthTest)
    {
		switch (depthTest)
		{
		case enum_depth_test::never:
			return EShaderDepthState::Never;
		case enum_depth_test::less:
			return EShaderDepthState::Less;
		case enum_depth_test::greater:
			return EShaderDepthState::Greater;
		case enum_depth_test::equal:
			return EShaderDepthState::Equal;
		case enum_depth_test::always:
			return EShaderDepthState::Always;
		default:
			return EShaderDepthState::Unknown;
		}
    }

    void ShaderModule::SetGlobalUniformBufferLayout(UniformBufferLayout* layout)
    {
		if (!GlobalUniformBufferLayout.IsValid()) [[unlikely]]
		{
			GlobalUniformBufferLayout = layout;
		}
    }

    void ShaderModule::SetPassUniformBufferLayout(EMeshPass pass, ShaderArchive* archive)
    {
		if (!PassUniformBufferLayoutMap.contains(pass)) [[unlikely]]
		{
			static String passUB = "Pass";
			ShaderParameterMap* param = new ShaderParameterMap();
			if (!archive->GenerateDefaultUBParameters(passUB, param))
			{
				delete param;
				return;
			}
			if (auto subShader = archive->GetSubShader(pass))
			{
				String subShadeName = subShader->GetName().ToString();
				PassUniformBufferLayoutMap[pass] = archive->GetUniformBufferLayout(subShadeName);
				PassDefaultParameterMap.emplace(pass, param);
			}
		}
    }

    const UniformBufferLayout* ShaderModule::GetPassUniformBufferLayout(EMeshPass pass)
    {
		auto it = GetModule()->PassUniformBufferLayoutMap.find(pass);
		if (it == GetModule()->PassUniformBufferLayoutMap.end()) [[unlikely]]
		{
			return nullptr;
		}
		return it->second;
    }

    ShaderParameterMap* ShaderModule::GetPassDefaultParameters(EMeshPass pass)
    {
		auto it = GetModule()->PassDefaultParameterMap.find(pass);
		if (it == GetModule()->PassDefaultParameterMap.end()) [[unlikely]]
		{
			return nullptr;
		}
		return it->second;
    }

    void ShaderModule::SetPrimitiveUniformBufferLayout(UniformBufferLayout* layout)
	{
		if (!PrimitiveUniformBufferLayout.IsValid()) [[unlikely]]
		{
			PrimitiveUniformBufferLayout = layout;
		}
    }

    ShaderBytecodeHash ShaderStage::GetBytecodeHash() const
	{
		ShaderBytecodeHash Hash;
		if (ByteCode.Size == 0)
		{
			Hash.Hash[0] = Hash.Hash[1] = 0;
		}
		else
		{
			// D3D shader bytecode contains a 128bit checksum in DWORD 1-4. We can just use that directly instead of hashing the whole shader bytecode ourselves.
			const uint8* pData = static_cast<uint8*>(ByteCode.Data) + 4;
			Hash = *reinterpret_cast<const ShaderBytecodeHash*>(pData);
		}
		return Hash;
	}

	uint32 ShaderCombination::GetTypeHash(const ShaderCombination& combination)
	{
		TArray<uint8> byteArray;
		byteArray.resize(combination.Shaders.size() * sizeof(uint64) * 3);
		size_t index = 0;
		for (const auto& ShaderStage : combination.Shaders | std::views::values)
		{
			size_t offset = index * sizeof(uint64) * 3;
			memcpy(byteArray.data() + offset, ShaderStage->GetBytecodeHash().Hash, sizeof(uint64) * 2);
			memcpy(byteArray.data() + offset + sizeof(uint64) * 2, &ShaderStage->VariantId, sizeof(uint64));
			index++;
		}
		return FCrc::StrCrc32(byteArray.data());
	}
}
