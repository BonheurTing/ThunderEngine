#include "PostProcessRenderer.h"

#include "CoreModule.h"
#include "FrameGraph.h"
#include "RenderMesh.h"
#include "RenderModule.h"
#include "ShaderArchive.h"
#include "ShaderModule.h"
#include "ShaderParameterMap.h"
#include "RHIResource.h"

namespace Thunder
{
    namespace
    {
        void Print(const std::string& message)
        {
            std::cout << message << std::endl;
        }
    }
    static NameHandle PassName_Blur = "Blur";

    void PostProcessRenderer::Setup(FrameGraph* frameGraph, PassOperations& operations)
    {
        if (Owner != frameGraph)
        {
            Owner = frameGraph;
        }
        // set param case1
        Parameter0 = 5.0f;

        frameGraph->AddPass(EVENT_NAME("Blur"), std::move(operations), [this]()
        {
            this->Execute();
        });
    }

    void PostProcessRenderer::Execute()
    {
        // set param case2
        Parameter1 = GConfigManager->GetConfig("BaseEngine")->GetBool("EnableRenderFeature0");

        // set up param
        auto context = Owner->GetMainContext();
        auto pass = Owner->GetPass(PassName_Blur); // or context->GetCurrentPass();
        auto archive = ShaderModule::GetShaderArchive("PostProcess");
        if (pass->bLayoutNeedsUpdate)
        {
            pass->BindingLayout = archive->GetBindingsLayout();
            pass->PassUBLayout = archive->GetUniformBufferLayout(PassName_Blur);
            if (pass->PassParameters == nullptr)
            {
                pass->PassParameters = new ShaderParameterMap;
            }
            else
            {
                pass->PassParameters->Reset();
            }
        }
        pass->PassParameters->SetFloatParameter("PassParameters0", Parameter0);
        pass->PassParameters->SetVectorParameter("PassParameters1", TVector4f(0.f, 0.f, 0.f, 1.f));
        pass->PassParameters->SetVectorParameter("PassParameters2", TVector4f(0.f, 0.f, 0.f, 1.f));
        pass->PassParameters->SetIntParameter("PassParameters3", Parameter3);

        static NameHandle sceneTextureName = "SceneTexture";
        auto rts = pass->GetFGTextures();
        auto sceneTex = rts.find(sceneTextureName);
        if (sceneTex != rts.end())
        {
            pass->PassParameters->SetTextureParameter(sceneTextureName, sceneTex->second);
        }

        byte* constantData = RenderModule::SetupUniformBufferParameters(context, pass->PassUBLayout, pass->PassParameters, PassName_Blur.ToString());

        if (pass->bLayoutNeedsUpdate)
        {
            if (pass->PassUniformBuffer.IsValid())
            {
                RHIDeferredDeleteResource(std::move(pass->PassUniformBuffer));
            }
            pass->PassUniformBuffer = RHICreateUniformBuffer(pass->PassUBLayout->GetTotalSize(), EUniformBufferFlags::UniformBuffer_SingleFrame, constantData);
        }
        else
        {
            RHIUpdateUniformBuffer(context, pass->PassUniformBuffer, constantData);
        }
        pass->bLayoutNeedsUpdate = false;
        
        // Dispatch command
        RHIDrawCommand* newCommand = new (context->Allocate<RHIDrawCommand>()) RHIDrawCommand;
        uint64 shaderVariant = 0; // todo
        newCommand->Shader = ShaderModule::GetShaderCombination(archive, PassName_Blur, shaderVariant);
        auto subMesh = GProceduralGeometryManager->GetGeometry(EProceduralGeometry::Triangle);
        newCommand->GraphicsPSO = RenderModule::GetPipelineState(context, PassName_Blur, archive, newCommand->Shader, subMesh);

        // Apply shader bindings.
        newCommand->Bindings.SetTransientAllocated(true);
        SingleShaderBindings* bindings = newCommand->Bindings.GetSingleShaderBindings();
        size_t const bindingSize = pass->BindingLayout->GetTotalSize();
        byte* bindingData = static_cast<byte*>(context->Allocate<byte>(bindingSize));
        memset(bindingData, 0, bindingSize);
        bindings->SetData(bindingData);
        auto frameGraph = context->GetFrameGraph();
        // Bind Constant Buffer
        {
            // Global uniform buffer.
            auto globalUB = frameGraph->GetGlobalUniformBuffer();
            if (globalUB == nullptr || globalUB->GetResource() == nullptr) [[unlikely]]
            {
                TAssertf(false, "Failed to get global CBV binding : buffer resource is invalid.");
            }

            static NameHandle globalUBName = "Global";
            bindings->SetUniformBuffer(pass->BindingLayout, globalUBName, { .Handle = reinterpret_cast<uint64>(globalUB) });

            // Pass uniform buffer.
            static NameHandle passUBName = "Pass";
            bindings->SetUniformBuffer(pass->BindingLayout, passUBName, { .Handle = reinterpret_cast<uint64>(pass->PassUniformBuffer.Get()) });
        }
        // Bind Texture
        {
            TGuid texGuid;
            pass->PassParameters->GetTextureParameter(sceneTextureName, texGuid);
            if (!texGuid.IsSystemValue()) [[unlikely]]
            {
                TAssertf(false, "Failed to get FGTexture binding : \"%s\", texture resource is invalid.", sceneTextureName.c_str());
                return;
            }

            RenderTextureRef rt = frameGraph->GetAllocatedRenderTarget(texGuid.D);
            if (!rt.IsValid()) [[unlikely]]
            {
                TAssertf(false, "Failed to get FGTexture binding : \"%s\", texture resource is invalid.", sceneTextureName.c_str());
                return;
            }

            // Get resource.
            RHITexture* textureResource = rt->GetTextureRHI();
            if (!textureResource) [[unlikely]]
            {
                TAssertf(false, "Failed to get SRV binding : \"%s\", texture resource is invalid.", sceneTextureName.c_str());
                return;
            }

            // Get SRV.
            RHIShaderResourceView* srv = textureResource->GetSRV();
            if (!srv) [[unlikely]]
            {
                TAssertf(false, "Failed to get SRV binding : \"%s\", SRV is invalid.", sceneTextureName.c_str());
                return;
            }

            // Bind.
            bindings->SetSRV(pass->BindingLayout, sceneTextureName, { .Handle = srv->GetOfflineHandle() });
        }
        // Bind Rtv
        {
            
        }

        context->AddCommand(newCommand);
        Print("Execute postprocess2");
    }
}
