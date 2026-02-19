#pragma optimize("", off)
#include "ShaderArchive.h"
#include <algorithm>
#include <charconv>
#include "Assertion.h"
#include "AstNode.h"
#include "CoreModule.h"
#include "MeshPass.h"
#include "ShaderLang.h"
#include "ShaderModule.h"
#include "ShaderParameterMap.h"
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
		void  GenerateParameterResourceCount(const TArray<InElementType>& param, uint8& outSRVNum, uint8& outUAVNum)
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

    ShaderPass::ShaderPass(class ShaderArchive* archive, const ast_node_pass* astNode)
		: Archive(archive), Name(astNode->get_name())
    {
    	auto const& attributes = astNode->get_attributes();
    	MeshPass = ShaderModule::GetMeshPass(attributes.mesh_draw_type);
    	if (MeshPass == EMeshPass::Num)
    	{
    		bMeshPass = false;
    	}
	    else
	    {
	    	bMeshPass = true;
	    }
    	BlendMode = ShaderModule::GetBlendMode(attributes.blend_mode);
    	DepthState = ShaderModule::GetDepthState(attributes.depth_test);
    	StencilState = EShaderStencilState::Default;
    }

    void ShaderPass::CacheDefaultShaderCache()
    {
    	//todo: gen default mask and default combination
    	//todo: compile default
    }

	void ShaderArchive::CalcRegisterCounts(NameHandle passName, TShaderRegisterCounts& outCount)
    {
		// Static Sampler is managed by the Root Signature and does not occupy the dynamic sampler table.
    	outCount.SamplerCount = 0;
    	outCount.ConstantBufferCount = static_cast<uint8>(GUniformBufferDefinitions.size());
    	outCount.ShaderResourceCount = 0;
    	outCount.UnorderedAccessCount = 0;
    	GenerateParameterResourceCount<ShaderPropertyMeta>(PropertyMeta, outCount.ShaderResourceCount, outCount.UnorderedAccessCount);
    	for (auto& uniformMetas : UniformParameterMeta)
    	{
    		if (GUniformBufferDefinitions.contains(uniformMetas.first))
    		{
    			GenerateParameterResourceCount<ShaderParameterMeta>(uniformMetas.second, outCount.ShaderResourceCount, outCount.UnorderedAccessCount);
    		}
    	}
    	QuantizeRegisterCounts(outCount);
    }

	void ShaderArchive::QuantizeRegisterCounts(TShaderRegisterCounts& outCount)
    {
    	// Root signatures are fixed for now, maybe deal with this later.
    	TAssert(outCount.ConstantBufferCount <= MAX_CBS, "Too many constant buffers are used, max allowed count is 16.");
		outCount.SamplerCount = 0;
		outCount.ShaderResourceCount = MAX_SRVS; // outCount.ShaderResourceCount > 0 ? MAX_SRVS : 0;
		outCount.UnorderedAccessCount = MAX_UAVS; // (outCount.UnorderedAccessCount > 0) ? MAX_UAVS : 0;
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
	    SyncCompilingVariantsLock.WriteLock();
	    {
    		// Check again.
	    	auto variantIt = SyncCompilingVariants.find(variantId);
	    	if (variantIt != SyncCompilingVariants.end()) [[unlikely]]
	    	{
	    		// Already compiling, fetch compiling status.
	    		syncCompilingEntry = SyncCompilingVariants[variantId];
	    		SyncCompilingVariantsLock.WriteUnlock();

	    		// Wait for sync compilation thread finishing compiling.
	    		auto variantLockGuard = syncCompilingEntry->Lock.Read();
	    		return syncCompilingEntry->Combination;
	    	}
	    }

    	// Launch a compilation task.
    	syncCompilingEntry = new (TMemory::Malloc<SyncCompilingCombinationEntry>()) SyncCompilingCombinationEntry;
    	SyncCompilingVariants[variantId] = syncCompilingEntry;
    	syncCompilingEntry->Lock.WriteLock();
    	SyncCompilingVariantsLock.WriteUnlock();
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
    	Variants[variantId] = MakeRefCount<ShaderCombination>(this);
    	ShaderCombination* newVariant = Variants[variantId];
    	for (auto& meta : StageMetas)
    	{
    		// Variant Marco
    		const uint64 stageVariantId = variantId & meta.second.VariantMask;
    		THashMap<NameHandle, bool> shaderMarco{};
    		Archive->VariantMaskToName(stageVariantId, shaderMarco);
    		//todo: include file
    		auto& shaderStageMap = newVariant->GetShaders();
    		ShaderStage* newStageVariant = new ShaderStage{};
    		shaderStageMap[meta.first] = newStageVariant;
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

    	ShaderCombination* newVariant = new (TMemory::Malloc<ShaderCombination>()) ShaderCombination{ this };
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

    		ShaderStage* newStageVariant = new (TMemory::Malloc<ShaderStage>()) ShaderStage{};
    		auto& shaderStageMap = newVariant->GetShaders();
    		shaderStageMap[stageType] = newStageVariant;

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

	void ShaderAST::Reflect(ShaderArchive* archive) const
	{
    	TAssert(ASTRoot->node_type == enum_ast_node_type::archive);
    	ast_node_archive* node = static_cast<ast_node_archive*>(ASTRoot);

    	shader_codegen_state state
		{
			.custom_types = CustomTypes,
		};

    	// Reflect properties.
    	for (ast_node_variable* var : node->properties)
    	{
    		ShaderPropertyMeta meta{};
    		meta.Name = var->name;
    		meta.Type = var->type->get_type_text(state);
    		meta.Format = var->type->get_type_format_text();
    		meta.Default = var->default_value;
    		archive->AddPropertyMeta(meta);
    	}

    	// Reflect variants.
    	for (ast_node_variable* var : node->variants)
    	{
    		ShaderVariantMeta meta{};
    		meta.Name = var->name;
    		meta.Default = var->default_value.empty() ? false : (var->default_value == "true" ? true : false);
    		archive->AddVariantMeta(meta);
    	}

    	// Reflect uniform buffers.
    	for (auto const& uniformBufferParametersEntry : node->uniform_buffer_parameters)
    	{
    		String const& uniformBufferName = uniformBufferParametersEntry.first;
    		TArray<ast_node_variable*> const& uniformBufferParameters = uniformBufferParametersEntry.second;
    		auto& metaList = archive->EnsureAndGetUniformBufferMetaList(uniformBufferName);
    		TAssertf(metaList.empty(), "Uniform buffer is not empty.");
    		for (ast_node_variable* uniformBufferParameter : uniformBufferParameters)
    		{
    			ShaderParameterMeta meta{};
    			meta.Name = uniformBufferParameter->name;
    			meta.Type = uniformBufferParameter->type->get_type_text(state);
    			meta.Format = uniformBufferParameter->type->get_type_format_text();
    			meta.Default = uniformBufferParameter->default_value;
    			metaList.push_back(meta);
    		}
    	}

    	// Reflect sampler parameters.
    	for (auto const* sampler : node->sampler_parameters)
    	{
    		archive->SamplerMeta.emplace_back(sampler->name);
    	}

    	// Reflect pass parameter
    	for (auto const& constantBufferParametersEntry : node->pass_cb_parameters)
    	{
    		String const& uniformBufferName = constantBufferParametersEntry.first;
    		TArray<ast_node_variable*> const& constantBufferParameters = constantBufferParametersEntry.second;
    		auto& metaList = archive->EnsureAndGetUniformBufferMetaList(uniformBufferName);
    		TAssertf(metaList.empty(), "Uniform buffer is not empty.");
    		for (ast_node_variable* constantBufferParameter : constantBufferParameters)
    		{
    			ShaderParameterMeta meta{};
    			meta.Name = constantBufferParameter->name;
    			meta.Type = constantBufferParameter->type->get_type_text(state);
    			meta.Format = constantBufferParameter->type->get_type_format_text();
    			meta.Default = constantBufferParameter->default_value;
    			metaList.push_back(meta);
    		}
    	}

    	// Reflect passes.
    	for (ast_node_pass* subShaderNode : node->passes)
    	{
    		// Parse sub-shader.
    		ShaderPass* subShader = new ShaderPass(archive, subShaderNode);
    		for (uint8 stage = 1; stage < static_cast<uint8>(enum_shader_stage::max); ++stage)
    		{
    			String entry = subShaderNode->get_stage_entry(static_cast<enum_shader_stage>(stage));
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
    	}

    	// Generate constant buffer layout
    	archive->BuildUniformBufferLayout();

    	// Generate global uniform buffer layout
    	ShaderModule::GetModule()->SetGlobalUniformBufferLayout(archive->GetUniformBufferLayout("Global"));
    	for (uint8 passIndex = 0; passIndex < static_cast<uint8>(EMeshPass::Num); ++passIndex)
    	{
    		ShaderModule::GetModule()->SetPassUniformBufferLayout(static_cast<EMeshPass>(passIndex), archive);
    	}
    	ShaderModule::GetModule()->SetPrimitiveUniformBufferLayout(archive);

    	// Generate bindings layout.
    	archive->BuildBindingsLayout();
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

    ShaderArchive::~ShaderArchive()
    {
    	if (AST)
    	{
    		TMemory::Free(AST);
    	}
    }

    void ShaderArchive::AddSubShader(ShaderPass* inSubShader)
    {
    	TShaderRegisterCounts quantizedRegisterCounts{};
    	CalcRegisterCounts(inSubShader->GetName(), quantizedRegisterCounts);
    	inSubShader->SetShaderRegisterCounts(quantizedRegisterCounts);
    	SubShaders.emplace(inSubShader->GetName(), ShaderPassRef(inSubShader));
    	if (inSubShader->IsMeshPass())
    	{
    		MeshDrawSubShaders.emplace(inSubShader->GetMeshPass(), ShaderPassRef(inSubShader));
    	}
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

	void ShaderArchive::BuildUniformBufferLayout()
	{
    	for (auto const& [uniformBufferName, parameterMetas] : UniformParameterMeta)
    	{
    		if (uniformBufferName == "Pass")
    		{
    			continue;
    		}
    		UniformBufferLayoutRef layout = new UniformBufferLayout{ this };
    		uint32 currentOffset = 0;

    		for (auto const& paramMeta : parameterMetas)
    		{
    			// Skip texture types as they are not part of uniform buffer.
    			if (paramMeta.Type.find("Texture") != std::string::npos)
    			{
    				continue;
    			}

    			UniformBufferMemberEntry entry;

    			// Determine type, size, and natural alignment.
    			EUniformBufferMemberType memberType = EUniformBufferMemberType::Num;
    			uint32 memberSize = 0;
    			uint32 memberAlignment = 4;

    			if (paramMeta.Type == "int")
    			{
    				memberType = EUniformBufferMemberType::Int;
    				memberSize = 4;
    				memberAlignment = 4;
    			}
    			else if (paramMeta.Type == "float")
    			{
    				memberType = EUniformBufferMemberType::Float;
    				memberSize = 4;
    				memberAlignment = 4;
    			}
    			else if (paramMeta.Type == "float4")
    			{
    				memberType = EUniformBufferMemberType::Float4;
    				memberSize = 16;
    				memberAlignment = 16;
    			}
    			else
    			{
    				TAssertf(false, "Unsupported uniform buffer member type \"%s\" for parameter \"%s\" in uniform buffer \"%s\".",
    					paramMeta.Type.c_str(), paramMeta.Name.c_str(), uniformBufferName.c_str());
    				continue;
    			}

    			// Align current offset to member's natural alignment.
    			currentOffset = (currentOffset + memberAlignment - 1) & ~(memberAlignment - 1);

    			// Set entry properties.
    			entry.Type = memberType;
    			entry.Size = memberSize;
    			entry.Offset = currentOffset;

    			// Add member to layout.
    			layout->AddMember(paramMeta.Name, entry);

    			// Advance offset.
    			currentOffset += memberSize;
    		}

    		// Align total size to 16 bytes.
    		constexpr uint32 kTotalAlignment = 16;
    		currentOffset = (currentOffset + kTotalAlignment - 1) & ~(kTotalAlignment - 1);
    		layout->SetTotalSize(currentOffset);

    		// Store layout.
    		UniformBufferLayoutMap[uniformBufferName] = layout;
    	}
	}

	void ShaderArchive::BuildBindingsLayout()
    {
    	constexpr uint16 kMaxSRVBindings = 16;
    	constexpr uint16 kMaxUAVBindings = 8;
    	uint16 currentSRVIndex = 0;
    	uint16 currentUAVIndex = 0;
    	BindingsLayout = new ShaderBindingsLayout{ this };

    	// Build Static Sampler bindings.
    	for (uint16 i = 0; i < static_cast<uint16>(SamplerMeta.size()); i++ )
    	{
    		BindingsLayout->AddBinding(
    		{
    			.Name = SamplerMeta[i],
    			.Index = i,
    			.Type = EShaderParameterType::Sampler,
    		});
    	}

    	// Build uniform buffers.
    	for (auto& uniformBufferNames : UniformParameterMeta | std::views::keys)
    	{
    		auto definitionIt = GUniformBufferDefinitions.find(uniformBufferNames);
    		if (definitionIt != GUniformBufferDefinitions.end()) // only GUniformBufferDefinitions.size cb
    		{
    			uint16 bufferIndex = definitionIt->second.index;
    			BindingsLayout->AddBinding(
				{
					.Name = uniformBufferNames,
					.Index = bufferIndex,
					.Type = EShaderParameterType::UniformBuffer,
				});
    		}
    	}

    	// Build UAVs and SRVs.
    	for (auto& uniformMetas : UniformParameterMeta)
    	{
    		if (!GUniformBufferDefinitions.contains(uniformMetas.first))
    		{
    			continue;
    		}
    		for (auto const& uniformBufferParameter : uniformMetas.second)
    		{
    			if (uniformBufferParameter.Type.find("Texture") != std::string::npos)
    			{
    				if (uniformBufferParameter.Type.starts_with("RW"))
    				{
    					if (currentUAVIndex < kMaxUAVBindings)
    					{
    						BindingsLayout->AddBinding(
							{
								.Name = uniformBufferParameter.Name,
								.Index = currentUAVIndex,
								.Type = EShaderParameterType::UAV,
							});
    						++currentUAVIndex;
    					}
					    else
					    {
						    TAssertf(false, "Can't bind more than %d UAVs.", static_cast<int>(kMaxUAVBindings));
					    }
    				}
    				else
    				{
    					if (currentSRVIndex < kMaxSRVBindings)
    					{
    						BindingsLayout->AddBinding(
							{
								.Name = uniformBufferParameter.Name,
								.Index = currentSRVIndex,
								.Type = EShaderParameterType::SRV,
							});
    						++currentSRVIndex;
    					}
    					else
    					{
    						TAssertf(false, "Can't bind more than %d SRVs.", static_cast<int>(kMaxSRVBindings));
    					}
    				}
    			}
    		}
    	}
	}

	UniformBufferLayout* ShaderArchive::GetUniformBufferLayout(NameHandle cbName)
	{
    	auto layoutIt = UniformBufferLayoutMap.find(cbName);
    	if (layoutIt != UniformBufferLayoutMap.end() && layoutIt->second.IsValid())
    	{
    		return layoutIt->second.Get();
    	}
	    return nullptr;
	}

	void ShaderArchive::GenerateDefaultParameters(const TArray<ShaderParameterMeta>& parameterMeta, ShaderParameterMap* shaderParameterMap)
	{
    	for (auto const& meta : parameterMeta)
    	{
    		if (meta.Type == "int")
    		{
    			int32 value = 0;
    			if (!meta.Default.empty())
    			{
    				auto const result = std::from_chars(meta.Default.data(), meta.Default.data() + meta.Default.size(), value);
    				if (result.ec != std::errc()) [[unlikely]]
    				{
    					TAssertf(false, "Failed to parse int parameter \"%s\" with default value \"%s\". Using default value 0.",
    						meta.Name.c_str(), meta.Default.c_str());
    					value = 0;
    				}
    			}
    			shaderParameterMap->IntParameters.emplace(meta.Name, value);
    		}
    		else if (meta.Type == "float")
    		{
    			float value = 0.0f;
    			if (!meta.Default.empty())
    			{
    				auto const result = std::from_chars(meta.Default.data(), meta.Default.data() + meta.Default.size(), value);
    				if (result.ec != std::errc()) [[unlikely]]
    				{
    					TAssertf(false, "Failed to parse float parameter \"%s\" with default value \"%s\". Using default value 0.0.",
    						meta.Name.c_str(), meta.Default.c_str());
    					value = 0.0f;
    				}
    			}
    			shaderParameterMap->FloatParameters.emplace(meta.Name, value);
    		}
    		else if (meta.Type == "float4")
    		{
    			TVector4f value(0.0f, 0.0f, 0.0f, 0.0f);
    			if (!meta.Default.empty())
    			{
    				// Support two formats:
    				// 1. float4(1, 2, 3, 4)
    				// 2. { 1.0, 2.0, 3.0, 4.0 }
    				String defaultStr = meta.Default;

    				// Find the content between delimiters
    				size_t startPos = defaultStr.find_first_of("({");
    				size_t endPos = defaultStr.find_last_of(")}");

    				if (startPos == String::npos || endPos == String::npos || startPos >= endPos) [[unlikely]]
					{
						TAssertf(false, "Failed to parse float4 parameter \"%s\" with default value \"%s\". Expected format: \"float4(x, y, z, w)\" or \"{ x, y, z, w }\". Using default value (0, 0, 0, 0).",
							meta.Name.c_str(), meta.Default.c_str());
						value = TVector4f(0.0f, 0.0f, 0.0f, 0.0f);
						continue;
    				}

    				// Extract the content between delimiters
    				String content = defaultStr.substr(startPos + 1, endPos - startPos - 1);

    				// Parse four floats separated by commas
    				float components[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    				size_t componentIndex = 0;
    				size_t parseStart = 0;

    				for (size_t i = 0; i <= content.size() && componentIndex < 4; ++i)
    				{
    					// Find comma or end of string
    					if (i == content.size() || content[i] == ',')
    					{
    						// Skip leading whitespace
    						while (parseStart < i && std::isspace(static_cast<unsigned char>(content[parseStart])))
    						{
    							++parseStart;
    						}

    						// Find end of number (skip trailing whitespace)
    						size_t parseEnd = i;
    						while (parseEnd > parseStart && std::isspace(static_cast<unsigned char>(content[parseEnd - 1])))
    						{
    							--parseEnd;
    						}

    						// Parse the float
    						if (parseStart < parseEnd)
    						{
    							auto const result = std::from_chars(
    								content.data() + parseStart,
    								content.data() + parseEnd,
    								components[componentIndex]);

    							if (result.ec != std::errc())
    							{
    								TAssertf(false, "Failed to parse float4 parameter \"%s\" component %zu with default value \"%s\". Using default value (0, 0, 0, 0).",
    									meta.Name.c_str(), componentIndex, meta.Default.c_str());
    								value = TVector4f(0.0f, 0.0f, 0.0f, 0.0f);
    								break;
    							}
    							++componentIndex;
    						}

    						parseStart = i + 1;
    					}
    				}

    				if (componentIndex == 4) [[likely]]
    				{
    					value = TVector4f(components[0], components[1], components[2], components[3]);
    				}
    				else
    				{
    					TAssertf(false, "Failed to parse float4 parameter \"%s\" with default value \"%s\". Expected 4 components, got %zu. Using default value (0, 0, 0, 0).",
    						meta.Name.c_str(), meta.Default.c_str(), componentIndex);
    					value = TVector4f(0.0f, 0.0f, 0.0f, 0.0f);
    				}
    			}
    			shaderParameterMap->VectorParameters.emplace(meta.Name, value);
    		}
    		else if (meta.Type.starts_with("Texture2D"))
    		{
    			shaderParameterMap->TextureParameters.emplace(meta.Name, TGuid());
    		}
		    else
		    {
			    TAssertf(false, "Parameter type \"%s\" not supported yet, parameter name is \"%s\".", meta.Type.c_str(), meta.Name.c_str());
		    }
    	}
	}

	String ShaderArchive::GenerateShaderSource(ShaderCodeGenConfig const& config) const
	{
    	shader_codegen_state state
    	{
    		.sub_shader_name = config.SubShaderName,
    		.variant_mask = config.VariantMask,
    		.stage = ShaderModule::GetShaderASTStage(config.Stage),
    		.custom_types = AST->GetCustomTypes(),
    		.variants = {}
    	};
    	ParseVariants(config, state);
    	return AST->GenerateShaderVariantSource(state);
    }

    String ShaderArchive::GetSubShaderEntry(String const& subShaderName, EShaderStageType stageType) const
    {
    	return AST->GetSubShaderEntry(subShaderName, stageType);
    }

    bool ShaderArchive::GenerateDefaultUBParameters(String const& ubName, ShaderParameterMap* shaderParameterMap)
    {
    	auto metaIt = UniformParameterMeta.find(ubName);
    	if (metaIt == UniformParameterMeta.end())
    	{
    		return false;
    	}
    	GenerateDefaultParameters(metaIt->second, shaderParameterMap);
    	return true;
    }

    void ShaderArchive::GenerateDefaultParameters(ShaderParameterMap* shaderParameterMap)
    {
    	for (auto meta : PropertyMeta)
    	{
    		if (meta.Type == "int")
    		{
    			shaderParameterMap->IntParameters.emplace(meta.Name, 0);
    		}
    		else if (meta.Type == "float")
    		{
    			shaderParameterMap->FloatParameters.emplace(meta.Name, 0.f);
    		}
    		else if (meta.Type == "float4")
    		{
    			shaderParameterMap->VectorParameters.emplace(meta.Name, TVector4f());
    		}
    		else if (meta.Type == "Texture2D")
    		{
    			shaderParameterMap->TextureParameters.emplace(meta.Name, TGuid());
    		}
    	}
    	for (auto const& meta : VariantMeta)
    	{
    		if (meta.Visible)
    		{
    			shaderParameterMap->StaticSwitchParameters.emplace(meta.Name, meta.Default);
    		}
    	}
    	for (auto& uniformMetas : UniformParameterMeta | std::views::values)
    	{
    		GenerateDefaultParameters(uniformMetas, shaderParameterMap);
    	}
    }
}