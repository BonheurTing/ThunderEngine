#pragma once
#include "CoreMinimal.h"

enum class EShaderStage : uint8
{
	Unknown	= 0,
	VertexShader,
	HullShader,
	DomainShader,
	PixelShader,
	ComputeShader,
	MeshShader
};

struct ShaderMeta
{
	
};

struct ShaderEntry
{
	BinaryData ByteCode;
	ShaderMeta MetaInfo;
};

struct ShaderCombination
{
	HashMap<EShaderStage, ShaderEntry> Shaders;
};

struct Pass
{

	
	HashMap<uint64, ShaderCombination> Variants;
};

class ShaderArchive
{
public:
private:
	HashMap<NameHandle, Pass> Passes;
};

class SHADER_API ShaderModule
{
public:
	static bool ParseShaderFile();
	
private:
	static HashMap<NameHandle, ShaderArchive> ShaderCache;
};
