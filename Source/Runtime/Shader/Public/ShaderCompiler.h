#pragma once
#include "CoreMinimal.h"

class Shader_API ICompiler
{
public:
	virtual void Compile(const String& inSource, BinaryData& outByteCode) = 0;
};

class Shader_API IDx11Compiler : public ICompiler
{
public:
	void Compile(const String& inSource, BinaryData& outByteCode) override;
};

class Shader_API IDx12Compiler : public ICompiler
{
public:
	void Compile(const String& inSource, BinaryData& outByteCode) override;
};