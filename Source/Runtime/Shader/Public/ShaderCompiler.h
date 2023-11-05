#pragma once
#include "CoreMinimal.h"
#include "dxcapi.h"

namespace Thunder
{
	class SHADER_API ICompiler
    {
    public:
    	virtual ~ICompiler() = default;
    	virtual void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const HashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, ManagedBinaryData& outByteCode) = 0;
    };
    
    class SHADER_API FXCCompiler : public ICompiler
    {
    public:
    	FXCCompiler();
    	void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const HashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, ManagedBinaryData& outByteCode) override;
    };
    
    class SHADER_API DXCCompiler : public ICompiler
    {
    public:
    	DXCCompiler();
    	void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const HashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, ManagedBinaryData& outByteCode) override;
    private:
    	ComPtr<IDxcUtils> ShaderUtils;
    	ComPtr<IDxcCompiler> ShaderCompiler;
    };
}
