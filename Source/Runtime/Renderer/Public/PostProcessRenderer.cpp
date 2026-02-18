#include "PostProcessRenderer.h"

#include "CoreModule.h"
#include "FrameGraph.h"
#include "RenderModule.h"
#include "ShaderArchive.h"
#include "ShaderModule.h"
#include "ShaderParameterMap.h"

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
        if (pass->bLayoutNeedsUpdate)
        {
            auto archive = ShaderModule::GetShaderArchive("PostProcess");
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
            //pass->Shader = GetShaderCombination(meshPassType, material);
            //pass->PSO = GetPipelineState(context, shaderVariant, meshPassType, subMesh, material);
        }
        pass->PassParameters->SetFloatParameter("PassParameters0", Parameter0);
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
        newCommand->Shader = pass->Shader;
        newCommand->GraphicsPSO = pass->PSO;

        // Apply shader bindings.
        newCommand->Bindings.SetTransientAllocated(true);
        SingleShaderBindings* bindings = newCommand->Bindings.GetSingleShaderBindings();
        size_t const bindingSize = pass->BindingLayout->GetTotalSize();
        byte* bindingData = static_cast<byte*>(context->Allocate<byte>(bindingSize));
        memset(bindingData, 0, bindingSize);
        bindings->SetData(bindingData);
        // Bind Constant Buffer
        {
            // Global uniform buffer.
            auto frameGraph = context->GetFrameGraph();
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

        context->AddCommand(newCommand);
        Print("Execute postprocess2");
    }
}
