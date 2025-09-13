#pragma once
#include "Concurrent/TaskGraph.h"

namespace Thunder
{
    class GameTask //临时放在这
    {
    public:
        friend class TTask<GameTask>;

        GameTask()
        {
        }

        void DoWork()
        {
            EngineLoop();
        }

        virtual void CustomBeginFrame() {}
        virtual void CustomEndFrame() {}

    private:
        void Init();
        void EngineLoop();
        void AsyncLoading();
        void GameMain();
		
    private:
        TaskGraphProxy* TaskGraph {};

        int ModelData[1024] { 0 };
        bool ModelLoaded[1024] { false };
    };
}

