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

	class RenderingTask //临时放在这 后续移到RenderCore
	{
	public:
		friend class TTask<RenderingTask>;

		int32 FrameData;
		RenderingTask() : FrameData(0)
		{
		}

		RenderingTask(int32 InExampleData)
		 : FrameData(InExampleData)
		{
		}

		void DoWork()
		{
			RenderMain();
		}
	private:
		void RenderMain();
		void SimulatingAddingMeshBatch();
	};

	class RHITask //临时放在这
	{
	public:
		friend class TTask<RHITask>;

		int32 FrameData;
		RHITask() : FrameData(0)
		{
		}

		RHITask(int32 InExampleData)
		 : FrameData(InExampleData)
		{
		}

		void DoWork()
		{
			RHIMain();
		}
	private:
		void RHIMain();
	};
}
