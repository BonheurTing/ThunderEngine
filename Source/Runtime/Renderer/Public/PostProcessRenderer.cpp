#include "PostProcessRenderer.h"

#include "CoreModule.h"
#include "FrameGraph.h"

namespace Thunder
{
    namespace
    {
        void Print(const std::string& message)
        {
            std::cout << message << std::endl;
        }
    }

    void PostProcessRenderer::Setup(FrameGraph* frameGraph, PassOperations& operations)
    {
        if (Owner != frameGraph)
        {
            Owner = frameGraph;
        }
        // set param case1
        Parameter0 = 5.0f;

        frameGraph->AddPass(EVENT_NAME("PostProcess2"), std::move(operations), [this]()
        {
            this->Execute();
        });
    }

    void PostProcessRenderer::Execute()
    {
        // set param case2
        Parameter1 = GConfigManager->GetConfig("BaseEngine")->GetBool("EnableRenderFeature0");

        //GetParameters()->SetFloat("Param0", Parameter0);
        //GetParameters()->SetFloat("Param1", Parameter1);

        auto context = Owner->GetMainContext();
        RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
        context->AddCommand(newCommand);
        Print("Execute postprocess2");
    }
}
