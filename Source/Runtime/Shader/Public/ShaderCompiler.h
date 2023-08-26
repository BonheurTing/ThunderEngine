#pragma once
#include "ShaderDefinition.h"
#include "dxcapi.h"

class SHADER_API ICompiler
{
public:
	virtual ~ICompiler() = default;
	virtual void Compile(const String& inSource, SIZE_T srcDataSize, const HashMap<NameHandle, bool>& marco, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) = 0;
};

class SHADER_API FXCCompiler : public ICompiler
{
public:
	FXCCompiler();
	void Compile(const String& inSource, SIZE_T srcDataSize, const HashMap<NameHandle, bool>& marco, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override;
};

class SHADER_API DXCCompiler : public ICompiler
{
public:
	DXCCompiler();
	void Compile(const String& inSource, SIZE_T srcDataSize, const HashMap<NameHandle, bool>& marco, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override;
private:
	ComPtr<IDxcUtils> ShaderUtils;
	ComPtr<IDxcCompiler> ShaderCompiler;
};

extern SHADER_API HashMap<EShaderStageType, String> GShaderModuleTarget;
extern SHADER_API ICompiler* GShaderCompiler;

FORCEINLINE void Compile(const String& inSource, const HashMap<NameHandle, bool>& marco, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode)
{
	GShaderCompiler->Compile(inSource, inSource.size(), marco, pEntryPoint, pTarget, outByteCode);
}