#pragma once
#include <wrl/client.h>

#include "BinaryData.h"
#include "CoreMinimal.h"
#include "dxcapi.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
	using namespace Microsoft::WRL;
	class SHADER_API ICompiler : public RefCountedObject
    {
    public:
    	virtual ~ICompiler() = default;
    	virtual void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco,
    		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) = 0;

		virtual void Compile(const String& inSource, SIZE_T srcDataSize, BinaryData& outByteCode) = 0;
    };
    
    class SHADER_API FXCCompiler : public ICompiler
    {
    public:
    	FXCCompiler();
    	void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco,
    		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override;

    	void Compile(const String& inSource, SIZE_T srcDataSize, BinaryData& outByteCode) override;
    };
    
    class SHADER_API DXCCompiler : public ICompiler
    {
    public:
    	DXCCompiler();
    	void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco,
    		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override;
    	void Compile(const String& inSource, SIZE_T srcDataSize, BinaryData& outByteCode) override {}
    private:
        ComPtr<IDxcUtils> ShaderUtils;
        ComPtr<IDxcCompiler> ShaderCompiler;
    };
}
