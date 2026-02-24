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

    void PostProcessManager::Render(const FGRenderTarget* inputRT)
    {
        // Post-process (present pass): reads LightingRT and renders directly to the swapchain backbuffer.
        // No write target is declared; the framegraph binds the backbuffer automatically.
        PassOperations operations;
        operations.Read(inputRT, "SceneTexture");

        // set param
        Parameter0 = 5.0f;

        Owner->AddPass(EVENT_NAME("ToneMapping"), std::move(operations), [this]()
        {
            auto pass = Owner->GetMainContext()->GetCurrentPass();
            pass->SetShader("PostProcess");

            // set param
            Parameter1 = GConfigManager->GetConfig("BaseEngine")->GetBool("EnableRenderFeature0");
            pass->PassParameters->SetFloatParameter("PassParameters0", Parameter0);
            pass->PassParameters->SetVectorParameter("PassParameters1", TVector4f(0.f, 0.f, 0.f, 1.f));
            pass->PassParameters->SetVectorParameter("PassParameters2", TVector4f(0.f, 0.f, 0.f, 1.f));
            pass->PassParameters->SetIntParameter("PassParameters3", Parameter3);

            auto subMesh = GProceduralGeometryManager->GetGeometry(EProceduralGeometry::Triangle);
            RenderModule::BuildDrawCommand(Owner, pass, subMesh);
        });
    }
}
