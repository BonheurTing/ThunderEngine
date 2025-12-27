#include "ShaderCompiler.h"
#include "Assertion.h"
#include "CoreModule.h"
#include "d3dcompiler.h"
#include "ShaderArchive.h"
#include "ShaderModule.h"
#include "FileSystem/FileModule.h"

namespace Thunder
{
	class ThunderID3DInclude : public ID3DInclude
    {
    public:
	    virtual ~ThunderID3DInclude() = default;
	    ThunderID3DInclude(const String& src, NameHandle name) : IncludeCode(src), ShaderName(name) {}
    	ThunderID3DInclude() = delete;
    	HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override
    	{
    		auto includedFilename = std::string(pFileName);
    		if (IncludeCode.empty())
    		{
    			return S_OK;
    		}
    		if (includedFilename == "Generated.tsh")
    		{
    			*ppData = IncludeCode.c_str();
    			*pBytes = static_cast<UINT>(IncludeCode.size());
    			return S_OK;
    		}
    		else
    		{
    			const ShaderArchive* archive = ShaderModule::GetShaderArchive(ShaderName);
    			const String fullPath = archive->GetShaderSourceDir() + "\\" + includedFilename;
    
    			const size_t pos = IncludeSourceList.size();
    			IncludeSourceList.push_back("");
    			auto& IncludeSource = IncludeSourceList[pos];
    			if (FileModule::LoadFileToString(fullPath, IncludeSource))
    			{
    				*ppData = IncludeSource.c_str();
    				*pBytes = static_cast<UINT>(IncludeSource.size());
    				return S_OK;
    			}
    			else
    			{
    				return S_FALSE;
    			}
    		}
    	}
    	HRESULT Close(LPCVOID pData) override { return S_OK; }
    private:
    	const String IncludeCode;
    	NameHandle ShaderName;
    	TArray<String> IncludeSourceList;
    };
    
    FXCCompiler::FXCCompiler()
    {
    	GShaderModuleTarget = {
    		{EShaderStageType::Vertex, "vs_5_0"},
    		{EShaderStageType::Pixel, "ps_5_0"},
    		{EShaderStageType::Compute, "cs_5_0"}
    	};
    }
    
    void FXCCompiler::Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode)
    {
    #if defined(SHAHER_DEBUG)
    	// Enable better shader debugging with the graphics debugging tools.
    	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
    	UINT compileFlags = 0;
    #endif
    	ComPtr<ID3DBlob> pResult, errorMessages;
    
    	D3D_SHADER_MACRO pDefines[64] = {};
    	int i = 0;
    	for (auto def : marco)
    	{
    		pDefines[i] = {def.first.c_str(), def.second ? "1" : "0"};
    		i++;
    	}
    
    	ThunderID3DInclude pInclude(includeStr, archiveName);
    	const HRESULT hr = D3DCompile(inSource.c_str(), srcDataSize,nullptr, pDefines, &pInclude, pEntryPoint.c_str(), pTarget.c_str(), compileFlags, 0, &pResult, &errorMessages);
    	//const HRESULT hr = D3DCompileFromFile(L"D:/ThunderEngine/Shader/shaders.hlsl",nullptr, nullptr, pEntryPoint.c_str(), pTarget.c_str(), 0, 0, &pResult, &errorMessages);
    	
    	if (SUCCEEDED(hr))
    	{
    		outByteCode.Size = pResult->GetBufferSize();
    		outByteCode.Data = TMemory::Malloc<uint8>(outByteCode.Size);
    		memcpy(outByteCode.Data, pResult->GetBufferPointer(), outByteCode.Size);
    	}
    	else
    	{
    		TAssertf(!errorMessages, "Compilation failed with errors: %s\n",static_cast<const char*>(errorMessages->GetBufferPointer()));
    		LOG("Fail to Compile Shader");
    	}
    }

    void FXCCompiler::Compile(const String& inSource, SIZE_T srcDataSize, BinaryData& outByteCode)
    {
    	
    }

    DXCCompiler::DXCCompiler()
    {
    	GShaderModuleTarget = {
    		{EShaderStageType::Vertex, "vs_6_0"},
    		{EShaderStageType::Pixel, "ps_6_0"},
    		{EShaderStageType::Compute, "cs_6_0"}
    	};
    	
    	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&ShaderUtils));
    	if (FAILED(hr))
    	{
    		LOG("Fail to Creat DXCLibrary");
    	}
    
    	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&ShaderCompiler));
    	if (FAILED(hr))
    	{
    		LOG("Fail to Creat DXCCompiler");
    	}
    }
    
    void DXCCompiler::Compile(NameHandle archiveName, const String& inSource, SIZE_T srcDataSize, const THashMap<NameHandle, bool>& marco, const String& includeStr, const String& pEntryPoint, const String& pTarget, BinaryData& outByteCode)
    {
    	if (ShaderCompiler == nullptr || ShaderUtils == nullptr)
    	{
    		LOG("ShaderCompiler or ShaderUtils is NULL");
    		return;
    	}
    
    	const std::wstring wEntryPoint = std::wstring(pEntryPoint.begin(), pEntryPoint.end());
    	const std::wstring wTarget = std::wstring(pTarget.begin(), pTarget.end());
    	
    	ComPtr<IDxcCompiler3> compiler3{};
    	ShaderCompiler->QueryInterface(IID_PPV_ARGS(&compiler3));
    	if(compiler3)
    	{
    		LPCWSTR pszArgs[] =
    			{
    			//L"",            // Optional shader source file name for error reporting and for PIX shader source view.  
    			L"-E", wEntryPoint.c_str(),         // Entry point.
    			L"-T", wTarget.c_str(),            // Target.
    			L"-Zi",                      // Enable debug information.
    			//L"-D", L"MYDEFINE=1",        // A single define.dxcapi.h 变体
    			// L"-Fo", L"myshader.bin",     // Optional. Stored in the pdb. 
    			// L"-Fd", L"myshader.pdb",     // The file name of the pdb. This must either be supplied or the autogenerated file name must be used.
    			L"-Qstrip_reflect",          // Strip reflection into a separate blob. 
    			L"-O3"                     // "-Od" 关优化
    			// -Qstrip_debug    -Qembed_debug
    		};
    
    		ComPtr<IDxcIncludeHandler> pIncludeHandler;
    		HRESULT hr = ShaderUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
    		if (FAILED(hr))
    		{
    			LOG("Fail to CreateDefaultIncludeHandler.");
    			return;
    		}
    	
    		DxcBuffer sourceBlob{};
    		sourceBlob.Ptr = inSource.c_str();
    		sourceBlob.Size = inSource.size();
    		ComPtr<IDxcResult> pResult;
    		hr = compiler3->Compile(&sourceBlob, pszArgs, _countof(pszArgs), pIncludeHandler.Get(), IID_PPV_ARGS(&pResult));
    		if (SUCCEEDED(hr))
    		{
    			hr = pResult->GetStatus(&hr);
    		}
    		if (SUCCEEDED(hr))
    		{
    			ComPtr<IDxcBlobUtf8> pErrors = nullptr;
    			pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
    			if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    			{
    				wprintf(L"DXCompiler %S\n", pErrors->GetStringPointer());
    				return;
    			}
    		}
    		else
    		{
    			LOG("Fail to Compile Shader");
    			return;
    		}
    
    		ComPtr<IDxcBlob> pShader = nullptr;
    		ComPtr<IDxcBlobUtf16> pShaderName = nullptr;
    		pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);
    		if (pShader != nullptr && pShader->GetBufferSize() > 0)
    		{
    			outByteCode.Size = pShader->GetBufferSize();
    			outByteCode.Data = pShader->GetBufferPointer();
    		}
    		else
    		{
    			LOG("Compile Empty Shader");
    		}
    	}
    	else
    	{
    		
    		const std::wstring wSource = std::wstring(inSource.begin(), inSource.end());
    		ComPtr<IDxcBlobEncoding> pBlobEncoding;
    		HRESULT hr = ShaderUtils->CreateBlob(wSource.c_str(), inSource.size(), 0, &pBlobEncoding);
    		if (FAILED(hr))
    		{
    			LOG("Fail to CreateBlob.");
    			return;
    		}
    		
    		ComPtr<IDxcOperationResult> pResult;
    		hr = ShaderCompiler->Compile(pBlobEncoding.Get(), nullptr, wEntryPoint.c_str(), wTarget.c_str(),
    			nullptr, 0, nullptr, 0, nullptr, &pResult);
    		if (SUCCEEDED(hr))
    		{
    			hr = pResult->GetStatus(&hr);
    		}
    		if (SUCCEEDED(hr))
    		{
    			ComPtr<IDxcBlob> pShader = nullptr;
    			pResult->GetResult(&pShader);
    			if (pShader != nullptr)
    			{
    				outByteCode.Size = pShader->GetBufferSize();
    				outByteCode.Data = pShader->GetBufferPointer();
    			}
    		}
    		else
    		{
    			ComPtr<IDxcBlobEncoding> errorsBlob;
    			hr = pResult->GetErrorBuffer(&errorsBlob);
    			if (SUCCEEDED(hr) && errorsBlob != nullptr)
    				wprintf(L"Warnings and Errors: %hs\n",static_cast<const char*>(errorsBlob->GetBufferPointer()));
    			LOG("Fail to Compile Shader");
    		}
    	}
    }

    void DXCCompiler::Compile(const String& inSource, const String& entryPoint, String const& target, BinaryData& outByteCode, bool debug/* = false*/)
    {
    	if (ShaderCompiler == nullptr || ShaderUtils == nullptr) [[unlikely]]
    	{
    		TAssertf(false, "Shader compiler not initialized.");
    		return;
    	}

    	const std::wstring wideEntry = std::wstring(entryPoint.begin(), entryPoint.end());
    	const std::wstring wideTarget = std::wstring(target.begin(), target.end());

    	ComPtr<IDxcCompiler3> compiler3{};
    	ShaderCompiler->QueryInterface(IID_PPV_ARGS(&compiler3));
    	if (compiler3)
    	{
    		TArray<LPCWSTR> args = {
    			L"-E", wideEntry.c_str(), // Entry point.
				L"-T", wideTarget.c_str(), // Target.
    			L"-Qstrip_reflect", // Strip reflection into a separate blob. 
    		};
    		if (debug)
    		{
    			args.push_back(L"-Zi");
    			args.push_back(L"-O3"); // "-Od" to disable optimization.
    		}
    		else
    		{
    			args.push_back(L"-O3");
    		}

    		ComPtr<IDxcIncludeHandler> pIncludeHandler;
    		HRESULT hr = ShaderUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
    		if (FAILED(hr))
    		{
    			LOG("Fail to CreateDefaultIncludeHandler.");
    			return;
    		}
    	
    		DxcBuffer sourceBlob{};
    		sourceBlob.Ptr = inSource.c_str();
    		sourceBlob.Size = inSource.size();
    		ComPtr<IDxcResult> pResult;
    		hr = compiler3->Compile(&sourceBlob, args.data(), static_cast<uint32>(args.size()), pIncludeHandler.Get(), IID_PPV_ARGS(&pResult));
    		if (SUCCEEDED(hr))
    		{
    			hr = pResult->GetStatus(&hr);
    		}
    		if (SUCCEEDED(hr))
    		{
    			ComPtr<IDxcBlobUtf8> pErrors = nullptr;
    			pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
    			if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    			{
    				TAssertf(false, "Fail to compile shader : %.*s", pErrors->GetStringLength(), pErrors->GetStringPointer());
    				wprintf(L"DXCompiler %S\n", pErrors->GetStringPointer());
    				outByteCode.Data = nullptr;
    				outByteCode.Size = 0;
    				return;
    			}
    		}
    		else
    		{
    			TAssertf(false, "Fail to compile shader, reason unknown.");
    			outByteCode.Data = nullptr;
    			outByteCode.Size = 0;
    			return;
    		}
    
    		ComPtr<IDxcBlob> pShader = nullptr;
    		ComPtr<IDxcBlobUtf16> pShaderName = nullptr;
    		pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);
    		if (pShader != nullptr && pShader->GetBufferSize() > 0)
    		{
    			outByteCode.Size = pShader->GetBufferSize();
    			outByteCode.Data = pShader->GetBufferPointer();
    		}
    		else
    		{
    			TAssertf(false, "Fail to get shader compilation result.");
    		}
    	}
    	else
    	{
    		const std::wstring wideSource = std::wstring(inSource.begin(), inSource.end());
    		ComPtr<IDxcBlobEncoding> blobEncoding;
    		HRESULT hr = ShaderUtils->CreateBlob(wideSource.c_str(), static_cast<uint32>(inSource.size()), 0, &blobEncoding);
    		if (FAILED(hr))
    		{
    			TAssertf(false, "Fail to create source blob when compiling a shader.");
    			return;
    		}

    		ComPtr<IDxcOperationResult> compileResult;
    		hr = ShaderCompiler->Compile(blobEncoding.Get(), nullptr, wideEntry.c_str(), wideTarget.c_str(),
    			nullptr, 0, nullptr, 0, nullptr, &compileResult);
    		if (SUCCEEDED(hr))
    		{
    			hr = compileResult->GetStatus(&hr);
    		}
    		if (SUCCEEDED(hr))
    		{
    			ComPtr<IDxcBlob> outShader = nullptr;
    			compileResult->GetResult(&outShader);
    			if (outShader != nullptr)
    			{
    				outByteCode.Size = outShader->GetBufferSize();
    				outByteCode.Data = outShader->GetBufferPointer();
    			}
    			else
    			{
    				TAssertf(false, "Fail to get shader compilation result.");
    			}
    		}
    		else
    		{
    			ComPtr<IDxcBlobEncoding> errorsBlob;
    			hr = compileResult->GetErrorBuffer(&errorsBlob);
    			if (SUCCEEDED(hr) && errorsBlob != nullptr)
    			{
    				TAssertf(false, "Fail to compile shader : %.*s", errorsBlob->GetBufferSize(), errorsBlob->GetBufferPointer());
    			}
    			else
    			{
    				TAssertf(false, "Fail to compile shader, reason unknown.");
    			}
    			outByteCode.Data = nullptr;
    			outByteCode.Size = 0;
    		}
    	}
    }
}

