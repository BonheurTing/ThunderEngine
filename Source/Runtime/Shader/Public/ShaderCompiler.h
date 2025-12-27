#pragma once
#include <wrl/client.h>

#include "BinaryData.h"
#include "CoreMinimal.h"
#include "dxcapi.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
	enum class EShaderStageType : uint8;
	using namespace Microsoft::WRL;
	class SHADER_API ICompiler : public RefCountedObject
    {
    public:
    	virtual ~ICompiler() = default;

    	virtual void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco,
    		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) = 0; // Deprecated.

		virtual void Compile(const String& inSource, SIZE_T srcDataSize, BinaryData& outByteCode) = 0; // Deprecated.

		virtual void Compile(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug = false) = 0;
    };
    
    class SHADER_API FXCCompiler : public ICompiler
    {
    public:
    	FXCCompiler();
    	void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco,
    		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override; // Deprecated.

    	void Compile(const String& inSource, SIZE_T srcDataSize, BinaryData& outByteCode) override; // Deprecated.

    	void Compile(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug = false) override {}
    };
    
    class SHADER_API DXCCompiler : public ICompiler
    {
    public:
    	DXCCompiler();
    	void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco,
    		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override; // Deprecated.
    	void Compile(const String& inSource, SIZE_T srcDataSize, BinaryData& outByteCode) override {} // Deprecated.
    	void Compile(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug = false) override;
    private:
        ComPtr<IDxcUtils> ShaderUtils;
        ComPtr<IDxcCompiler> ShaderCompiler;
    };
}
