#pragma optimize("", off)
#include "ShaderArchive.h"
#include <algorithm>
#include <sstream>
#include "Assertion.h"
#include "AstNode.h"
#include "CoreModule.h"
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
    }

    /*uint64 ShaderPass::VariantNameToMask(const TArray<ShaderVariantMeta>& variantName) const
    {
    	if (!variantName.empty())
    	{
    		uint64 mask = 0;
    		const int totalVariantNum = static_cast<int>(VariantDefinitionTable.size());
    		for (auto& meta : variantName)
    		{
    			int i = 0;
    			for (; i < totalVariantNum; i++)
    			{
    				if (meta.Name == VariantDefinitionTable[i].Name)
    				{
    					mask = mask | 1 << i;
    					break;
    				}
    			}
    			TAssertf(i < totalVariantNum, "Couldn't find variant %s", meta.Name);
    		}
    		return mask;
    	}
    	return 0;
    }*/

    uint64 ShaderPass::VariantNameToMask(const TMap<NameHandle, bool>& parameters) const
    {
    	uint64 mask = 0;
    	const int totalVariantNum = static_cast<int>(VariantDefinitionTable.size());
    	for (int i = 0; i < totalVariantNum; i++)
    	{
    		if (parameters.contains(VariantDefinitionTable[i].Name) && parameters.at(VariantDefinitionTable[i].Name))
    		{
    			mask = mask | (1ULL << i);
    		}
    	}
    	return mask;
    }

    void ShaderPass::VariantIdToShaderMarco(uint64 variantId, uint64 variantMask, THashMap<NameHandle, bool>& shaderMarco) const
    {
    	const int totalVariantNum = static_cast<int>(VariantDefinitionTable.size());
    	TAssertf(variantId >> totalVariantNum == 0, "Error input VariantId");
    	for (int i = 0; i < totalVariantNum; i++)
    	{
    		if (variantMask & (1ULL << i))
    		{
    			shaderMarco[VariantDefinitionTable[i].Name] = (variantId & (1ULL << i)) > 0;
    		}
    	}
    }

    void ShaderPass::SetVariantDefinitionTable(const TArray<ShaderVariantMeta>& passVariantMeta)
    {
    	PassVariantMask = 0;
    	int i = 0;
    	for (auto& meta : passVariantMeta)
    	{
    		VariantDefinitionTable.push_back(meta);
    		PassVariantMask = PassVariantMask | (1ULL << i);
    		i++;
    	}
    }

    void ShaderPass::GenerateVariantDefinitionTable_Deprecated(const TArray<ShaderVariantMeta>& passVariantMeta, const THashMap<EShaderStageType, TArray<ShaderVariantMeta>>& stageVariantMeta)
    {
    	// gen VariantDefinitionTable
    	/*THashSet<NameHandle> variantDefinition;
    	for (auto& meta : passVariantMeta)
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
    	for (auto& meta:  StageMetas)
    	{
    		meta.second.VariantMask = sharedStageMask;
    		if (stageVariantMeta.contains(meta.first))
    		{
    			meta.second.VariantMask = meta.second.VariantMask | VariantNameToMask( stageVariantMeta.at(meta.first) );
    		}
    	}*/
    }

    void ShaderPass::CacheDefaultShaderCache()
    {
    	//todo: gen default mask and default combination
    	//todo: compile default
    }
    
    void ShaderArchive::GenerateIncludeString(NameHandle passName, String& outFile)
    {
    	const bool hasPassParameters = PasseParameterMeta.contains(passName);
    	//check duplicate names
    	THashSet<NameHandle> uniqueNames;
    	for (auto& meta : PropertyMeta)
    	{
    		uniqueNames.insert(meta.Name);
    	}
    	for (auto& meta : GlobalParameterMeta)
    	{
    		uniqueNames.insert(meta.Name);
    	}
    	if (hasPassParameters)
    	{
    		for (auto& meta : PasseParameterMeta[passName])
    		{
    			uniqueNames.insert(meta.Name);
    		}
    	}
    	const bool hasDuplicateNames = uniqueNames.size() != PropertyMeta.size() + GlobalParameterMeta.size()
    		+ (hasPassParameters ? PasseParameterMeta[passName].size() : 0);
    	TAssertf(!hasDuplicateNames, "The parameters of the shader defines repeat");
    	if (hasDuplicateNames)
    	{
    		return;
    	}
    	// preprocess param
    	TArray<CBParamBinding> cbGeneratedCode;
    	TArray<String> sbGeneratedCode;
    	uint8 dummy = 0;
    	GenerateParameterCodeArray<ShaderPropertyMeta>(PropertyMeta, cbGeneratedCode, sbGeneratedCode, dummy);
    	GenerateParameterCodeArray<ShaderParameterMeta>(GlobalParameterMeta, cbGeneratedCode, sbGeneratedCode, dummy);
    	if (hasPassParameters)
    	{
    		GenerateParameterCodeArray<ShaderParameterMeta>(PasseParameterMeta[passName], cbGeneratedCode, sbGeneratedCode, dummy);
    	}
    	const bool hasInvalidParam = uniqueNames.size() != cbGeneratedCode.size() + sbGeneratedCode.size();
    	TAssertf(!hasInvalidParam, "The shader has invalid parameter definitions");
    	if (hasInvalidParam)
    	{
    		return;
    	}
    
    	std::stringstream codeStream;
    	// cb
    	if (!cbGeneratedCode.empty())
    	{
    		codeStream << "cbuffer GeneratedConstantBuffer : register(b0)\n{\n";
    	
    		std::sort(cbGeneratedCode.begin(), cbGeneratedCode.end(), CBParamBinding::cmp);
    		TDeque<CBParamBinding> cbQueue;
    		cbQueue.assign(cbGeneratedCode.begin(), cbGeneratedCode.end());
    
    		//todo: this is temporary algorithm to be optimized
    		int paddingNum = 0;
    		while (!cbQueue.empty())
    		{
    			CBParamBinding& cur = cbQueue.front();
    			codeStream << cur.GenCode;
    			cbQueue.pop_front();
    			int sizeToAlign = cur.Size;
    			while (sizeToAlign % 16 != 0)
    			{
    				const int lackSize = 16 - sizeToAlign % 16;
    				if (!cbQueue.empty())
    				{
    					CBParamBinding& pad = cbQueue.back();
    					if (pad.Size <= lackSize)
    					{
    						codeStream << pad.GenCode;
    						cbQueue.pop_back();
    						sizeToAlign += pad.Size;
    						continue;
    					}
    				}
    				codeStream << "\tfloat" << lackSize / 4 << " padding" << paddingNum << ";\n";
    				paddingNum++;
    				break;
    			}
    		}
    		codeStream << "}\n";
    	}
    	// sb & texture
    	int registerIndex = 0;
    	for (String& code : sbGeneratedCode)
    	{
    		codeStream << code << " : register(t" << registerIndex << ");\n";
    		registerIndex++;
    	}
    	// sampler
    	{
    		codeStream << "SamplerState GPointClampSampler : register(s0);\n";
    		codeStream << "SamplerState GPointWrapSampler : register(s1);\n";
    		codeStream << "SamplerState GLinearClampSampler : register(s2);\n";
    		codeStream << "SamplerState GLinearWrapSampler : register(s3);\n";
    		codeStream << "SamplerState GAnisotropicClampSampler : register(s4);\n";
    		codeStream << "SamplerState GAnisotropicWrapSampler : register(s5);\n";
    	}
    	
    	outFile =  codeStream.str();
    }
    
	void ShaderArchive::CalcRegisterCounts(NameHandle passName, TShaderRegisterCounts& outCount)
    {
    	outCount.SamplerCount = 6;
    	outCount.ConstantBufferCount = 1;

    	TArray<CBParamBinding> cbGeneratedCode;
    	TArray<String> sbGeneratedCode;
    	outCount.UnorderedAccessCount = 0;
    	GenerateParameterCodeArray<ShaderPropertyMeta>(PropertyMeta, cbGeneratedCode, sbGeneratedCode, outCount.UnorderedAccessCount);
    	GenerateParameterCodeArray<ShaderParameterMeta>(GlobalParameterMeta, cbGeneratedCode, sbGeneratedCode, outCount.UnorderedAccessCount);
    	if (PasseParameterMeta.contains(passName))
    	{
    		GenerateParameterCodeArray<ShaderParameterMeta>(PasseParameterMeta[passName], cbGeneratedCode, sbGeneratedCode, outCount.UnorderedAccessCount);
    	}
    	outCount.ShaderResourceCount = static_cast<uint8>(sbGeneratedCode.size()) - outCount.UnorderedAccessCount;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    /// Compile Shader
    //////////////////////////////////////////////////////////////////////////////////////////
    bool ShaderPass::CompileShader(NameHandle archiveName, const String& shaderSource, const String& includeStr, uint64 variantId)
    {
    	TAssertf((PassVariantMask & variantId) == variantId, "Compile Shader: Invalid variantId");
    	// Stage
    	Variants[variantId] = MakeRefCount<ShaderCombination>();
    	ShaderCombination& newVariant = *Variants[variantId];
    	for (auto& meta : StageMetas)
    	{
    		// Variant Marco
    		TAssertf((meta.second.VariantMask & PassVariantMask) == meta.second.VariantMask, "Compile Shader: Invalid stage variantId");
    		const uint64 stageVariantId = variantId & meta.second.VariantMask;
    		THashMap<NameHandle, bool> shaderMarco{};
    		VariantIdToShaderMarco(stageVariantId, meta.second.VariantMask, shaderMarco);
    		//todo: include file
    		newVariant.Shaders[meta.first] = new ShaderStage{};
    		ShaderStage* newStageVariant = newVariant.Shaders[meta.first];
    		ShaderModule::GetModule()->Compile(archiveName, shaderSource, shaderMarco, includeStr, meta.second.EntryPoint.c_str(), GShaderModuleTarget[meta.first], *newStageVariant->ByteCode);
    
    		if (newStageVariant->ByteCode->Size == 0)
    		{
    			TAssertf(false, "Compile Shader: Output an empty ByteCode");
    			return false;
    		}
    		newStageVariant->VariantId = stageVariantId;
    	}
    	return true;
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
    			meta.Default = var->default_value;
    			archive->AddGlobalParameterMeta(meta);
    		}
    		for (ast_node_variable* var : node->object_parameters)
    		{
    			ShaderParameterMeta meta{};
    			meta.Name = var->name;
    			meta.Type = var->type->get_type_text();
    			meta.Default = var->default_value;
    			archive->AddObjectParameterMeta(meta);
    		}
    		TArray<ShaderParameterMeta> dummyPassMeta{};
    		for (ast_node_variable* var : node->pass_parameters)
    		{
    			ShaderParameterMeta meta{};
    			meta.Name = var->name;
    			meta.Type = var->type->get_type_text();
    			meta.Default = var->default_value;
    			dummyPassMeta.push_back(meta);
    		}
    		for (ast_node_pass* var : node->passes)
    		{
    			//parse subshader
    			ShaderPass* currentPass = new ShaderPass(var->name);
    			currentPass->SetVariantDefinitionTable(archive->VariantMeta);
    			archive->AddSubShader(currentPass);
    			archive->AddPassParameterMeta(var->name, dummyPassMeta);
    		}
    	}
	}

	String ShaderAST::GenerateShaderVariantSource(NameHandle passName, const TArray<ShaderVariantMeta>& variants)
	{
    	String result = "";
    	ASTRoot->generate_hlsl(result);
    	return result;
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

    ShaderPass* ShaderArchive::GetPass(NameHandle name)
    {
    	if (Passes.contains(name))
    	{
    		return Passes[name].Get();
    	}
    	TAssertf(false, "Pass not exist");
    	return nullptr;
    }

    ShaderPass* ShaderArchive::GetSubShader(EMeshPass meshPassType)
    {
    	if (SubShaders.contains(meshPassType))
    	{
    		return SubShaders[meshPassType].Get();
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
	
    bool ShaderArchive::CompileShaderPass(NameHandle passName, uint64 variantId, bool force)
    {
    	ShaderPass* targetPass = GetPass(passName);
    	if (!force && targetPass && targetPass->CheckCache(variantId))
    	{
    		return true;
    	}
    	// SourcePath
    	const String fileName = FileModule::GetEngineRoot() + "\\Shader\\" + SourcePath;
    	String shaderSource;
    	FileModule::LoadFileToString(fileName, shaderSource);
    	// include file
    	String shaderIncludeStr;
    	GenerateIncludeString(passName, shaderIncludeStr);
    	if (shaderIncludeStr.empty())
    	{
    		return false;
    	}
    	return targetPass->CompileShader(Name, shaderSource, shaderIncludeStr, variantId);
    }

    String ShaderArchive::GenerateShaderSource(NameHandle passName, uint64 variantId)
    {
    	return AST->GenerateShaderVariantSource(passName, {});
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