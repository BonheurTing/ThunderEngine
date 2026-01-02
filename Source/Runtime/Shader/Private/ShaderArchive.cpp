#pragma optimize("", off)
#include "ShaderArchive.h"
#include <algorithm>
#include <sstream>
#include "Assertion.h"
#include "AstNode.h"
#include "CoreModule.h"
#include "ShaderLang.h"
#include "ShaderModule.h"
#include "FileSystem/FileModule.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
	//////////////////////////////////////////////////////////////////////////////////////////
    /// Parse Shader File
    //////////////////////////////////////////////////////////////////////////////////////////
    THashMap<EShaderStageType, String> GShaderModuleTarget = {};
    THashMap<String, String> GShaderParameterType = {
    	{"Integer", "int"},
    	{"Float", "float"},
    	{"Float4", "float4"},
    	{"Float4x4", "float4x4"},
    	{"Color", "float4"},
    	{"Texture2D", "Texture2D"}
    };
    THashMap<String, int> GShaderParameterSize = {
    	{"Integer", 4},
    	{"Float", 4},
    	{"Float4", 16},
    	{"Float4x4", 64},
    	{"Color", 16}
    };
    
    struct CBParamBinding
    {
    	String GenCode;
    	int Size; // 16-byte alignment
    
    	static bool cmp(const CBParamBinding& x, const CBParamBinding& y)
    	{
    		return x.Size > y.Size;
    	}
    };
    
    namespace
    {
    
    	template<typename InElementType>
    	void GenerateParameterCode(const InElementType& param, String& outCode)
    	{
    		const bool isTypeValid = !param.Name.IsEmpty() && !param.Type.empty() &&  GShaderParameterType.contains(param.Type);
    		TAssertf(isTypeValid, "Invalid input Parameter");
    		if (!isTypeValid)
    		{
    			outCode = "";
    			return;
    		}
    		String paramType = GShaderParameterType[param.Type];
    		std::stringstream code;
    		if (strstr(paramType.c_str(), "Texture"))
    		{
    			const bool isFormatValid = param.Format.empty() || GShaderParameterType.contains(param.Format);
    			TAssertf(isFormatValid, "Invalid input Parameter texture format");
    			if (!isFormatValid)
    			{
    				return;
    			}
    			const String format = param.Format.empty() ? "float4" : GShaderParameterType[param.Format];
    			code << paramType << "<" << format << "> " << param.Name.c_str();
    		}
    		else
    		{
    			const bool extendDefaultCode = strstr(paramType.c_str(), "float") && paramType.size() > 5;
    			if (param.Default.empty())
    			{
    				code << "\t" << paramType << " " << param.Name.c_str() << ";\n";
    			}
    			else
    			{
    				code << "\t" << paramType << " " << param.Name.c_str() << " = "
    				<< (extendDefaultCode ? paramType : "") << param.Default << ";\n";
    			}
    		}
    		
    		outCode = code.str();
    	}
    
    	template<typename InElementType>
    	void GenerateParameterCodeArray(const TArray<InElementType>& param, TArray<CBParamBinding>& outCbCode, TArray<String>& outSbCode, uint8& outUAVNum)
    	{
    		for (auto& meta : param)
    		{
    			if (strstr(meta.Type.c_str(), "Texture"))
    			{
    				String sbCode;
    				GenerateParameterCode<InElementType>(meta, sbCode);
    				if (!sbCode.empty())
    				{
    					outSbCode.push_back(sbCode);
    				}

    				if (strstr(meta.Type.c_str(), "RWTexture"))
    				{
    					outUAVNum++;
    				}
    			}
    			else
    			{
    				CBParamBinding cbCode{};
    				GenerateParameterCode<InElementType>(meta, cbCode.GenCode);
    				if (!cbCode.GenCode.empty())
    				{
    					// GenCode is not empty == invalid type, so GShaderParameterSize can be used directly
    					cbCode.Size = GShaderParameterSize[meta.Type];
    					outCbCode.push_back(cbCode);
    				}
    			}
    		}
    	}

    	template<typename InElementType>
		void GenerateParameterResourceCount(const TArray<InElementType>& param, uint8& outSRVNum, uint8& outUAVNum)
    	{
    		for (auto& meta : param)
    		{
    			if (strstr(meta.Type.c_str(), "Texture"))
    			{
    				if (strstr(meta.Type.c_str(), "RWTexture"))
    				{
    					outUAVNum++;
    				}
    				else
    				{
    					outSRVNum++;
    				}
    			}
    		}
    	}
    }

    void ShaderPass::CacheDefaultShaderCache()
    {
    	//todo: gen default mask and default combination
    	//todo: compile default
    }

	void ShaderArchive::CalcRegisterCounts(NameHandle passName, TShaderRegisterCounts& outCount)
    {
    	outCount.SamplerCount = 6;
    	outCount.ConstantBufferCount = 1;
    	outCount.ShaderResourceCount = 0;
    	outCount.UnorderedAccessCount = 0;
    	GenerateParameterResourceCount<ShaderPropertyMeta>(PropertyMeta, outCount.ShaderResourceCount, outCount.UnorderedAccessCount);
    	GenerateParameterResourceCount<ShaderParameterMeta>(GlobalParameterMeta,  outCount.ShaderResourceCount, outCount.UnorderedAccessCount);
    	GenerateParameterResourceCount<ShaderParameterMeta>(ObjectParameterMeta,  outCount.ShaderResourceCount, outCount.UnorderedAccessCount);
    	if (PasseParameterMeta.contains(passName))
    	{
    		GenerateParameterResourceCount<ShaderParameterMeta>(PasseParameterMeta[passName], outCount.ShaderResourceCount, outCount.UnorderedAccessCount);
    	}
    }

	ShaderCombination* ShaderPass::GetOrCompileShaderCombination(uint64 variantId)
    {
    	// Variants map quick path.
	    {
	    	auto lock = VariantsLock.Read();
	    	auto variantIt = Variants.find(variantId);
	    	if (variantIt != Variants.end())
	    	{
	    		return variantIt->second.Get();
	    	}
    	}

    	// Sync-compilation deduplication.
	    SyncCompilingCombinationEntry* syncCompilingEntry = nullptr;
	    {
    		auto syncCompilingMapLock = SyncCompilingVariantsLock.Read();
	    	auto variantIt = SyncCompilingVariants.find(variantId);
	    	if (variantIt != SyncCompilingVariants.end()) [[unlikely]]
	    	{
	    		// Already compiling, fetch compiling status.
	    		syncCompilingEntry = SyncCompilingVariants[variantId];
	    	}
	    }

    	// Deduplication hit.
    	if (syncCompilingEntry) [[unlikely]]
    	{
    		// Wait for sync compilation thread finishing compiling.
    		auto variantLockGuard = syncCompilingEntry->Lock.Read();
    		return syncCompilingEntry->Combination;
    	}

    	// Deduplication miss.
	    auto syncCompilingMapLock = SyncCompilingVariantsLock.Write();
	    {
    		// Check again.
	    	auto variantIt = SyncCompilingVariants.find(variantId);
	    	if (variantIt != SyncCompilingVariants.end()) [[unlikely]]
	    	{
	    		// Already compiling, fetch compiling status.
	    		syncCompilingEntry = SyncCompilingVariants[variantId];
	    		syncCompilingMapLock.Unlock();
	    		
	    		// Wait for sync compilation thread finishing compiling.
	    		auto variantLockGuard = syncCompilingEntry->Lock.Read();
	    		return syncCompilingEntry->Combination;
	    	}
	    }

    	// Launch a compilation task.
    	syncCompilingEntry = new (TMemory::Malloc<SyncCompilingCombinationEntry>()) SyncCompilingCombinationEntry;
    	SyncCompilingVariants[variantId] = syncCompilingEntry;
    	syncCompilingEntry->Lock.WriteLock();
    	syncCompilingMapLock.Unlock();
    	syncCompilingEntry->Combination = ShaderModule::CompileShaderVariant(Archive->GetName(), GetName(), variantId);
    	syncCompilingEntry->Lock.WriteUnlock();

    	// Write back to variants cache.
	    auto variantsLock = VariantsLock.Write();
    	TAssertf(!Variants.contains(variantId), "Duplicated variant found.");
    	Variants[variantId] = syncCompilingEntry->Combination;

    	return syncCompilingEntry->Combination;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    /// Compile Shader
    //////////////////////////////////////////////////////////////////////////////////////////
    bool ShaderPass::CompileShader(NameHandle archiveName, const String& shaderSource, const String& includeStr, uint64 variantId)
    {
    	TAssertf(false, "ShaderPass::CompileShader is deprecated.");
    	// Stage
    	Variants[variantId] = MakeRefCount<ShaderCombination>();
    	ShaderCombination& newVariant = *Variants[variantId];
    	for (auto& meta : StageMetas)
    	{
    		// Variant Marco
    		const uint64 stageVariantId = variantId & meta.second.VariantMask;
    		THashMap<NameHandle, bool> shaderMarco{};
    		Archive->VariantMaskToName(stageVariantId, shaderMarco);
    		//todo: include file
    		newVariant.Shaders[meta.first] = new ShaderStage{};
    		ShaderStage* newStageVariant = newVariant.Shaders[meta.first];
    		ShaderModule::GetModule()->Compile(archiveName, shaderSource, shaderMarco, includeStr, meta.second.EntryPoint.c_str(), GShaderModuleTarget[meta.first], newStageVariant->ByteCode);
    
    		if (newStageVariant->ByteCode.Size == 0)
    		{
    			TAssertf(false, "Compile Shader: Output an empty ByteCode");
    			return false;
    		}
    		newStageVariant->VariantId = stageVariantId;
    	}
    	return true;
    }

    ShaderCombination* ShaderPass::CompileShaderVariant(uint64 variantId)
    {
#if WITH_EDITOR
    	bool constexpr enableDebugInfo = true;
#else
    	bool constexpr enableDebugInfo = false;
#endif

    	ShaderCombination* newVariant = new (TMemory::Malloc<ShaderCombination>()) ShaderCombination;
    	for (auto& stageMetaIt : StageMetas)
    	{
    		EShaderStageType stageType = stageMetaIt.first;
    		StageMeta& stageMeta = stageMetaIt.second;

    		// Code-gen.
    		String source = Archive->GenerateShaderSource(ShaderCodeGenConfig
			{
				.SubShaderName = GetName(),
				.VariantMask = variantId,
				.Stage = stageType,
			});

    		newVariant->Shaders[stageType] = new (TMemory::Malloc<ShaderStage>()) ShaderStage{};
    		ShaderStage* newStageVariant = newVariant->Shaders[stageType];

    		String entryName = stageMeta.EntryPoint;
    		ShaderModule::CompileShaderSource(source, entryName, stageType, newStageVariant->ByteCode, enableDebugInfo);
    		if (!newStageVariant->ByteCode.Data
    			|| newStageVariant->ByteCode.Size == 0)
    		{
    			TMemory::Destroy(newStageVariant);
    		}
    		newStageVariant->VariantId = variantId;
    	}
		return newVariant;
    }

    ShaderAST::~ShaderAST()
    {
    	if (ASTRoot)
    	{
    		delete ASTRoot;
    		ASTRoot = nullptr;
    	}
    }

	void ShaderAST::ParseAllTypeParameters(ShaderArchive* archive) const
	{
    	TAssert(ASTRoot->node_type == enum_ast_node_type::archive);
    	if (ast_node_archive* node = static_cast<ast_node_archive*>(ASTRoot))
    	{
    		for (ast_node_variable* var : node->properties)
    		{
    			ShaderPropertyMeta meta{};
    			meta.Name = var->name;
    			meta.Type = var->type->get_type_text();
    			meta.Format = var->type->get_type_format_text();
    			meta.Default = var->default_value;
    			archive->AddPropertyMeta(meta);
    		}
    		for (ast_node_variable* var : node->variants)
    		{
    			ShaderVariantMeta meta{};
    			meta.Name = var->name;
    			meta.Default = var->default_value.empty() ? false : (var->default_value == "true" ? true : false);
    			archive->AddVariantMeta(meta);
    		}
    		for (ast_node_variable* var : node->global_parameters)
    		{
    			ShaderParameterMeta meta{};
    			meta.Name = var->name;
    			meta.Type = var->type->get_type_text();
    			meta.Format = var->type->get_type_format_text();
    			meta.Default = var->default_value;
    			archive->AddGlobalParameterMeta(meta);
    		}
    		for (ast_node_variable* var : node->object_parameters)
    		{
    			ShaderParameterMeta meta{};
    			meta.Name = var->name;
    			meta.Type = var->type->get_type_text();
    			meta.Format = var->type->get_type_format_text();
    			meta.Default = var->default_value;
    			archive->AddObjectParameterMeta(meta);
    		}
    		TArray<ShaderParameterMeta> dummyPassMeta{};
    		for (ast_node_variable* var : node->pass_parameters)
    		{
    			ShaderParameterMeta meta{};
    			meta.Name = var->name;
    			meta.Type = var->type->get_type_text();
    			meta.Format = var->type->get_type_format_text();
    			meta.Default = var->default_value;
    			dummyPassMeta.push_back(meta);
    		}
    		for (ast_node_pass* sub_shader_node : node->passes)
    		{
    			// Parse sub-shader.
    			ShaderPass* subShader = new ShaderPass(archive, sub_shader_node->name);
    			for (uint8 stage = 1; stage < static_cast<uint8>(enum_shader_stage::max); ++stage)
    			{
    				String entry = sub_shader_node->get_stage_entry(static_cast<enum_shader_stage>(stage));
    				if (!entry.empty())
    				{
    					EShaderStageType engineStage = ShaderModule::GetShaderStage(static_cast<enum_shader_stage>(stage));
    					subShader->AddStageMeta(engineStage, 
    					{
    						.EntryPoint = entry,
    						.VariantMask = 0
    					});
    				}
    			}
    			archive->AddSubShader(subShader);
    			archive->AddPassParameterMeta(sub_shader_node->name, dummyPassMeta);
    		}
    	}
	}

	String ShaderAST::GenerateShaderVariantSource(shader_codegen_state& state) const
    {
    	String result;
    	ASTRoot->generate_hlsl(result, state);
    	return result;
	}

	String ShaderAST::GetSubShaderEntry(String const& subShaderName, EShaderStageType stageType) const
	{
    	// Find sub-shader.
		TAssert(ASTRoot->node_type == enum_ast_node_type::archive);
		ast_node_archive* archiveNode = static_cast<ast_node_archive*>(ASTRoot);
    	ast_node_pass* subShaderNode = nullptr;
		for (ast_node_pass* pass : archiveNode->passes)
		{
			if (pass->name == subShaderName)
			{
				subShaderNode = pass;
				break;
			}
		}

    	if (!subShaderNode)
    	{
    		TAssertf(false, "Sub-shader \"%s\" not found.", subShaderName.c_str());
    		return "";
    	}

    	enum_shader_stage astStage = ShaderModule::GetShaderASTStage(stageType);
    	return subShaderNode->get_stage_entry(astStage);
	}

	ShaderArchive::ShaderArchive(String sourceFilePath, NameHandle shaderName, ast_node* astRoot)
	    : SourcePath(std::move(sourceFilePath)), Name(shaderName)
    {
    	AST = new (TMemory::Malloc<ShaderAST>()) ShaderAST(astRoot);
    	AST->ParseAllTypeParameters(this);
    }

    ShaderArchive::~ShaderArchive()
    {
    	if (AST)
    	{
    		TMemory::Free(AST);
    	}
    }

    void ShaderArchive::AddSubShader(ShaderPass* inSubShader)
    {
    	TShaderRegisterCounts counts{};
    	CalcRegisterCounts(inSubShader->GetName(), counts);
    	inSubShader->SetShaderRegisterCounts(counts);
    		
    	SubShaders.emplace(inSubShader->GetName(), ShaderPassRef(inSubShader));
    	MeshDrawSubShaders.emplace(EMeshPass::BasePass, ShaderPassRef(inSubShader)); // 12.28Todo : Get mesh-draw type.
    }

    ShaderPass* ShaderArchive::GetSubShader(NameHandle name)
    {
    	auto subShaderIt = SubShaders.find(name);
    	if (subShaderIt != SubShaders.end())
    	{
    		return subShaderIt->second;
    	}
    	TAssertf(false, "Pass \"%s\" not found in archive \"%s\".", name.c_str(), Name.c_str());
    	return nullptr;
    }

    ShaderPass* ShaderArchive::GetSubShader(EMeshPass meshPassType)
    {
    	if (MeshDrawSubShaders.contains(meshPassType))
    	{
    		return MeshDrawSubShaders[meshPassType].Get();
    	}
    	return nullptr;
    }

    String ShaderArchive::GetShaderSourceDir() const
    {
    	String fullPath = FileModule::GetEngineShaderRoot();
    	constexpr size_t invalidPosition = 0xffffffff;
    	size_t lastSlashPosition = invalidPosition;
    	if (SourcePath.find_last_of("\\") != std::string::npos)
    	{
    		lastSlashPosition = SourcePath.find_last_of("\\");
    	}
    	if (SourcePath.find_last_of('/') != std::string::npos)
    	{
    		lastSlashPosition =  std::max(lastSlashPosition, SourcePath.find_last_of('/'));
    	}
    	if (lastSlashPosition == invalidPosition)
    	{
    		return fullPath;
    	}
    	else
    	{
    		return fullPath + "\\" + SourcePath.substr(0, lastSlashPosition-1);
    	}
    }
	
    ShaderCombination* ShaderArchive::CompileShaderVariant(NameHandle subShaderName, uint64 variantId)
    {
    	// Retrieve sub-shader.
    	ShaderPass* subShader = GetSubShader(subShaderName);
    	if (!subShader) [[unlikely]]
		{
			TAssertf(false, "Failed to compile shader : sub-shader \"%s\" not find in archive \"%s\".", subShaderName.c_str(), Name.c_str());
			return nullptr;
		}
    	return subShader->CompileShaderVariant(variantId);
    }

	void ShaderArchive::ParseVariants(ShaderCodeGenConfig const& config, shader_codegen_state& state) const
	{
    	uint64 variantMask = config.VariantMask;

    	// Fixed variants.
    	for (uint64 variantBit = EFixedVariant_GlobalVariantsBegin; variantBit < 64; ++variantBit)
    	{
    		auto fixedVariantIt = GFixedVariantMap.find(static_cast<EFixedVariant>(1ull << variantBit));
    		if (fixedVariantIt != GFixedVariantMap.end())
    		{
    			NameHandle globalVariantName = fixedVariantIt->second;
    			state.variants[globalVariantName] = !!(variantMask & variantBit);
    		}
		    else
		    {
			    break;
		    }
    	}

    	// Shader variants.
    	VariantMaskToName(variantMask, state.variants);
    }

	uint64 ShaderArchive::VariantNameToMask(const TMap<NameHandle, bool>& variantMap) const
    {
    	uint64 mask = 0;
    	const int totalVariantNum = static_cast<int>(VariantMeta.size());
    	for (int variantBitIndex = 0; variantBitIndex < totalVariantNum; ++variantBitIndex)
    	{
    		NameHandle variantName = VariantMeta[variantBitIndex].Name;
    		auto variantIt = variantMap.find(variantName);
    		if (variantIt != variantMap.end() && variantIt->second)
    		{
    			mask = mask | (1ULL << variantBitIndex);
    		}
    	}
    	return mask;
	}

	void ShaderArchive::VariantMaskToName(uint64 variantMask, THashMap<NameHandle, bool>& variantMap) const
    {
    	const int totalVariantNum = static_cast<int>(VariantMeta.size());
    	TAssertf(((variantMask & 0xFFFFFFFF) >> totalVariantNum) == 0, "Variant mask contains undefined bits, archive name \"%s\".", GetName().c_str());
    	for (int variantBitIndex = 0; variantBitIndex < totalVariantNum; variantBitIndex++)
    	{
    		NameHandle variantName = VariantMeta[variantBitIndex].Name;
    		variantMap[variantName] = !!(variantMask & (1ULL << variantBitIndex));
    	}
	}

	String ShaderArchive::GenerateShaderSource(ShaderCodeGenConfig const& config) const
	{
    	shader_codegen_state state
    	{
    		.sub_shader_name = config.SubShaderName,
    		.variant_mask = config.VariantMask,
    		.stage = ShaderModule::GetShaderASTStage(config.Stage),
    		.variants = {}
    	};
    	ParseVariants(config, state);
    	return AST->GenerateShaderVariantSource(state);
    }

    String ShaderArchive::GetSubShaderEntry(String const& subShaderName, EShaderStageType stageType) const
    {
    	return AST->GetSubShaderEntry(subShaderName, stageType);
    }

    void AddParameters(const TArray<ShaderParameterMeta>& parameterMeta, MaterialParameterCache& cache)
    {
    	for (auto meta : parameterMeta)
    	{
    		if (meta.Type == "int")
    		{
    			cache.FloatParameters.emplace(meta.Name, 0);
    		}
    		else if (meta.Type == "float")
    		{
    			cache.VectorParameters.emplace(meta.Name, 0.f);
    		}
    		else if (meta.Type == "float4")
    		{
    			cache.VectorParameters.emplace(meta.Name, TVector4f());
    		}
    		else if (meta.Type == "Texture2D")
    		{
    			cache.TextureParameters.emplace(meta.Name, TGuid());
    		}
    	}
    }
	
    MaterialParameterCache ShaderArchive::GenerateParameterCache()
    {
    	MaterialParameterCache newVisibleParameters;
    	for (auto meta : PropertyMeta)
    	{
    		if (meta.Type == "int")
    		{
    			newVisibleParameters.FloatParameters.emplace(meta.Name, 0);
    		}
    		else if (meta.Type == "float")
    		{
    			newVisibleParameters.VectorParameters.emplace(meta.Name, 0.f);
    		}
    		else if (meta.Type == "float4")
    		{
    			newVisibleParameters.VectorParameters.emplace(meta.Name, TVector4f());
    		}
    		else if (meta.Type == "Texture2D")
    		{
    			newVisibleParameters.TextureParameters.emplace(meta.Name, TGuid());
    		}
    	}
    	for (auto meta : VariantMeta)
    	{
    		if (meta.Visible)
    		{
    			newVisibleParameters.StaticParameters.emplace(meta.Name, meta.Default);
    		}
    	}
    	AddParameters(GlobalParameterMeta, newVisibleParameters);
    	AddParameters(ObjectParameterMeta, newVisibleParameters);
    	for (auto val : PasseParameterMeta | std::views::values)
    	{
    		AddParameters(val, newVisibleParameters);
    	}
    	return newVisibleParameters;
    }


	
}
#pragma optimize("", on)