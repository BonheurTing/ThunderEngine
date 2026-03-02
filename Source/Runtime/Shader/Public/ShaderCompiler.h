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
	class ICompiler : public RefCountedObject
    {
    public:
    	SHADER_API virtual ~ICompiler() = default;

    	SHADER_API virtual void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco,
    		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) = 0; // Deprecated.

		SHADER_API virtual void Compile(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug = false) = 0;
    };
    
    class FXCCompiler : public ICompiler
    {
    public:
    	SHADER_API FXCCompiler();
    	SHADER_API void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco,
    		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override; // Deprecated.

    	SHADER_API void Compile(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug = false) override {}
    };
    
    class DXCCompiler : public ICompiler
    {
    public:
    	SHADER_API DXCCompiler();
    	SHADER_API void Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco,
    		const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode) override; // Deprecated.

    	SHADER_API void Compile(const String& inSource, const String& entryPoint, EShaderStageType stage, BinaryData& outByteCode, bool debug = false) override;
    private:
        ComPtr<IDxcUtils> ShaderUtils;
        ComPtr<IDxcCompiler> ShaderCompiler;
    };
}
