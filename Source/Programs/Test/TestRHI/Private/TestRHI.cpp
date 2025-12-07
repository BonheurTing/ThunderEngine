#include <d3d12.h>
#pragma optimize("", off)

#include "Assertion.h"
#include "IDynamicRHI.h"
#include "ShaderModule.h"
#include "D3D11RHIModule.h"
#include "D3D12RHIModule.h"
#include "EngineMain.h"
#include "CoreModule.h"
#include "CoreMinimal.h"
#include "Config/ConfigManager.h"
#include "FileSystem/FileModule.h"

namespace Thunder
{
    class TD3D11RHIModule;
    class TD3D12RHIModule;
}
using namespace Thunder;

int main()
{
    EngineMain MainEntry;
    ModuleManager::GetInstance()->LoadModule<CoreModule>();
    ModuleManager::GetInstance()->LoadModule<ShaderModule>();
    std::cout << "ThunderEngine Path: " << FileModule::GetEngineRoot() << std::endl;

    // test config
    String configRHIType = GConfigManager->GetConfig("BaseEngine")->GetString("RHI");
    EGfxApiType rhiType = EGfxApiType::Invalid;
    if (configRHIType == "DX11")
    {
        rhiType = EGfxApiType::D3D11;
    }
    else if (configRHIType == "DX12")
    {
        rhiType = EGfxApiType::D3D12;
    }
    else
    {
        TAssertf(false, "Invalid RHI Type");
    }
    if (!MainEntry.RHIInit(rhiType))
    {
        return 1;
    }
    std::cout << IRHIModule::GetModule()->GetName().c_str() << std::endl;
    
    //test RHI
    RHICreateDevice();
    IRHIModule::GetModule()->InitCommandContext();
    
    struct Vertex
    {
        TVector3f Position;
        TVector4f Color;
    };
    constexpr float aspectRatio = 1920.f / 1080;
    Vertex triangleVertices[] =
    {
        { { 0.f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };
    uint16 indices[] = { 0, 1, 2 };
    constexpr uint32 vertexBufferSize = sizeof(triangleVertices);
    RHICreateVertexBuffer(vertexBufferSize, sizeof(Vertex), EBufferCreateFlags::Dynamic, triangleVertices);
    RHICreateIndexBuffer( 3, ERHIIndexBufferType::Uint16, EBufferCreateFlags::Dynamic, indices);
    //RHICreateStructuredBuffer();
    auto cbvDesc = RHIResourceDescriptor::Buffer(10);
    cbvDesc.Format = RHIFormat::R32G32B32A32_FLOAT;
    const auto cbv = RHICreateConstantBuffer(256, EBufferCreateFlags::None);
    RHICreateConstantBufferView(*cbv, 256);
    
    RHIResourceDescriptor desc {};
    desc.Type = ERHIResourceType::Texture1D;
    desc.Alignment = 0;
    desc.Width = 64;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 3;
    desc.Format = RHIFormat::R32G32B32A32_FLOAT;
    desc.SampleDesc = {1, 0};
    desc.Layout = ERHITextureLayout::RowMajor;
    desc.Flags = {0, 0, 0, 0, 0};
    if(const auto resource = RHICreateTexture(desc, ETextureCreateFlags::None))
    {
        RHICreateShaderResourceView(*resource, {desc.Format, ERHIViewDimension::Texture1D, desc.Width, desc.Height, desc.DepthOrArraySize});
        std::cout << "Succeed to Create Texture1D and its SRV" << std::endl;
    }
    desc.Type = ERHIResourceType::Texture2D;
    desc.Height = 64;
    desc.Flags = {0, 0, 0, 1, 0};
    if(const auto resource = RHICreateTexture(desc, ETextureCreateFlags::None))
    {
        RHICreateRenderTargetView(*resource, {desc.Format, ERHIViewDimension::Texture2D, desc.Width, desc.Height, desc.DepthOrArraySize});
        std::cout << "Succeed to Create Texture2D and its RTV" << std::endl;
    }
    desc.Type = ERHIResourceType::Texture2DArray;
    desc.DepthOrArraySize = 2;
    desc.Flags = {0, 0, 1, 0, 0};
    if(const auto resource = RHICreateTexture(desc, ETextureCreateFlags::None))
    {
        RHICreateUnorderedAccessView(*resource, {desc.Format, ERHIViewDimension::Texture2DArray, desc.Width, desc.Height, desc.DepthOrArraySize});
        std::cout << "Succeed to Create Texture2DArray and its UAV" << std::endl;
    }
    desc.Type = ERHIResourceType::Texture3D;
    if(RHICreateTexture(desc, ETextureCreateFlags::None))
    {
        std::cout << "Succeed to Create Texture3D" << std::endl;
    }
    //todo: RHICreateSampler()
    RHICreateFence(0, 0);

    // Test Shader Compiler
    const String fileName = FileModule::GetEngineRoot() + "\\Shader\\shaders.hlsl";
    String shaderSource;
    FileModule::LoadFileToString(fileName, shaderSource);

    // Test Parse Shader File
    ShaderModule* shaderModule = ShaderModule::GetModule();
    if (shaderModule->ParseShaderFile_Deprecated())
    {
        std::cout << "Succeed to Parse Shader File" << std::endl;
    }
    if (shaderModule->CompileShaderCollection("PBR", "GBuffer", 0))
    {
        std::cout << "Succeed to Hot Reload Shaders" << std::endl;
    }

    // test create pso
    {
        TGraphicsPipelineStateDescriptor psoDesc;
        bool ret = shaderModule->GetPassRegisterCounts("Triangle", "ScreenPass", psoDesc.RegisterCounts);
        TArray<RHIVertexElement> inputElements =
        {
            { ERHIVertexInputSemantic::Position, 0, RHIFormat::R32G32B32_FLOAT, 0, 0, false },
            { ERHIVertexInputSemantic::Color, 0, RHIFormat::R32G32B32A32_FLOAT, 0, 12, false }
        };
        psoDesc.VertexDeclaration = RHIVertexDeclarationDescriptor{inputElements};
        ret = shaderModule->CompileShaderCollection("Triangle", "ScreenPass", 0);
        TAssertf(ret, "Failed to compile shader.");
        psoDesc.ShaderIdentifier = "Triangle|ScreenPass|0";
        psoDesc.RenderTargetFormats[0] = RHIFormat::R8G8B8A8_UNORM;
        psoDesc.DepthStencilFormat = RHIFormat::UNKNOWN;
        psoDesc.NumSamples = 1;
        psoDesc.PrimitiveType = ERHIPrimitiveType::Triangle;
        psoDesc.RenderTargetsEnabled = 1;
        if (auto pso = RHICreateGraphicsPipelineState(psoDesc))
        {
            std::cout << "Succeed to create pso" << std::endl;
        }
    }


    //exit
    MainEntry.~EngineMain();
    switch (rhiType)
    {
    case EGfxApiType::D3D12:
        ModuleManager::GetInstance()->UnloadModule<TD3D12RHIModule>();
        break;
    case EGfxApiType::D3D11:
        ModuleManager::GetInstance()->UnloadModule<TD3D11RHIModule>();
        break;
    case EGfxApiType::Invalid:
        break;
    }
    ModuleManager::GetInstance()->UnloadModule<ShaderModule>();
    ModuleManager::GetInstance()->UnloadModule<CoreModule>();
    ModuleManager::DestroyInstance();
    //system("pause");
    return 0;
}

