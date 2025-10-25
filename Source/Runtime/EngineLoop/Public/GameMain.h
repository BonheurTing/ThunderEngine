#pragma once
#include "Concurrent/TaskGraph.h"

namespace Thunder
{
    class GameTask
    {
    public:
        friend class TTask<GameTask>;

        GameTask();
        ~GameTask();

        void DoWork();

        virtual void CustomBeginFrame() {}
        virtual void CustomEndFrame() {}

    private:
        void AsyncLoading();
        void GameMain();
		
    private:
        TaskGraphProxy* TaskGraph {};

        int ModelData[1024] { 0 };
        bool ModelLoaded[1024] { false };
    };
}

