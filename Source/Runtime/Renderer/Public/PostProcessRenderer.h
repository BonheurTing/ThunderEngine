#pragma once

namespace Thunder
{
    class FrameGraph;
    class PostProcessRenderer
    {
    public:
        void Setup(FrameGraph* frameGraph, class PassOperations& operations);

        void Execute();
    private:
        float Parameter0 = 0.f;
        float Parameter1 = 0.f;
        float Parameter2 = 0.f;

        FrameGraph* Owner = nullptr;
    };
}

