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
            std::cout << message << '\n';
        }
    }

    void PostProcessManager::Setup(PassOperations& operations)
    {
        // set param case1
        Parameter0 = 5.0f;

        Owner->AddPass(EVENT_NAME("Blur"), std::move(operations), [this]()
        {
            this->Execute();
        });
    }

    void PostProcessManager::Execute()
    {
        auto context = Owner->GetMainContext();
        if (context == nullptr) [[unlikely]]
        {
            TAssertf(false, "Context is null");
            return;
        }
        auto pass = context->GetCurrentPass(); // "Blur"
        if (pass == nullptr) [[unlikely]]
        {
            TAssertf(false, "CurrentPass is null");
            return;
        }

        bool success = pass->SetShader("PostProcess");
        if (!success) [[unlikely]]
        {
            TAssertf(false, "Failed to set shader.");
            return;
        }

        // set param case2
        Parameter1 = GConfigManager->GetConfig("BaseEngine")->GetBool("EnableRenderFeature0");
        pass->PassParameters->SetFloatParameter("PassParameters0", Parameter0);
        pass->PassParameters->SetVectorParameter("PassParameters1", TVector4f(0.f, 0.f, 0.f, 1.f));
        pass->PassParameters->SetVectorParameter("PassParameters2", TVector4f(0.f, 0.f, 0.f, 1.f));
        pass->PassParameters->SetIntParameter("PassParameters3", Parameter3);

        auto subMesh = GProceduralGeometryManager->GetGeometry(EProceduralGeometry::Triangle);
        RenderModule::BuildDrawCommand(Owner, pass, subMesh);
        Print("Execute postprocess2");
    }
}
