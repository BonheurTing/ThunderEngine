#pragma once
#include "Vector.h"

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
        TVector4f Parameter1;
        TVector4f Parameter2;
        int Parameter3 = 0;

        FrameGraph* Owner = nullptr;
    };
}

