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
	uint64 VariantMask;
	BinaryData ByteCode;
	ShaderMeta MetaInfo;
};

struct ShaderCombination
{
	Map<EShaderStage, ShaderEntry> Shaders;
};

struct Pass
{
	Map<uint64, ShaderCombination> Variants; //uint64: VariantMask 
};

class ShaderArchive
{
public:
private:
	Map<NameHandle, Pass> Passes; //todo:NameHandle
};

