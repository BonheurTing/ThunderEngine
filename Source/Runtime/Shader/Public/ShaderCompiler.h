#pragma once
#include "CoreMinimal.h"
#include "dxcapi.h"

class SHADER_API ICompiler
{
public:
	virtual ~ICompiler() = default;
	virtual void Compile(const String& inSource, SIZE_T srcDataSize, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) = 0;
};

class SHADER_API FXCCompiler : public ICompiler
{
public:
	void Compile(const String& inSource, SIZE_T srcDataSize, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override;
};

class SHADER_API DXCCompiler : public ICompiler
{
public:
	DXCCompiler();
	void Compile(const String& inSource, SIZE_T srcDataSize, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override;
private:
	ComPtr<IDxcUtils> ShaderUtils;
	ComPtr<IDxcCompiler> ShaderCompiler;
};

extern SHADER_API ICompiler* GShaderCompiler;

FORCEINLINE void Compile(const String& inSource, SIZE_T srcDataSize, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode)
{
	GShaderCompiler->Compile(inSource, srcDataSize, pEntryPoint, pTarget, outByteCode);
}