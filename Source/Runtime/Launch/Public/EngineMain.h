#pragma once
#include "Launch.export.h"
#include "Misc/Task.h"
#include "Misc/TaskGraph.h"

namespace Thunder
{
	struct LAUNCH_API SimpleLock
	{
		std::mutex mtx;
		std::condition_variable cv;
	};

	extern LAUNCH_API bool GIsRequestingExit;
	extern LAUNCH_API SimpleLock* GThunderEngineLock;

	class PhysicsTask : public TaskGraphTask //临时放在这
	{
	public:
		int32 Data;
		PhysicsTask(): Data(0)
		{
		}

		PhysicsTask(int32 InPhysicsTaskData, const String& InDebugName = "")
			: TaskGraphTask(InDebugName)
			, Data(InPhysicsTaskData)
		{
		}

	private:
		void DoWorkInner() override;
	};

	class CullTask : public TaskGraphTask //临时放在这
	{
	public:
		int32 Data;
		CullTask(): Data(0)
		{
		}

		CullTask(int32 InCullTaskData, const String& InDebugName = "")
			: TaskGraphTask(InDebugName)
			, Data(InCullTaskData)
		{
		}

	private:
		void DoWorkInner() override;
	};

	class TickTask : public TaskGraphTask //临时放在这
	{
	public:
		int32 Data;
		TickTask(): Data(0)
		{
		}

		TickTask(int32 InTickTaskData, const String& InDebugName = "")
			: TaskGraphTask(InDebugName)
			, Data(InTickTaskData)
		{
		}

	private:
		void DoWorkInner() override;
		
	};
	
	class GameThread //临时放在这
	{
	public:
		friend class TTask<GameThread>;

		GameThread(int32 InExampleData)
		 : FrameData(InExampleData)
		{
			Init();
		}

		void DoWork()
		{
			EngineLoop();
		}
	private:
		void EngineLoop();
		void GameMain();
		void Init();
	private:
		int32 FrameData;
		TaskGraphProxy* TaskGraph;
	};

	class RenderingThread //临时放在这
	{
	public:
		friend class TTask<RenderingThread>;

		int32 FrameData;
		RenderingThread(): FrameData(0)
		{
		}

		RenderingThread(int32 InExampleData)
		 : FrameData(InExampleData)
		{
		}

		void DoWork()
		{
			RenderMain();
		}
	private:
		void RenderMain();
	};

	class RHIThreadTask //临时放在这
	{
	public:
		friend class TTask<RHIThreadTask>;

		int32 FrameData;
		RHIThreadTask(): FrameData(0)
		{
		}

		RHIThreadTask(int32 InExampleData)
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
	
	class LAUNCH_API EngineMain
    {
    public:
    	EngineMain(){}

		int32 FastInit();
    	bool RHIInit(EGfxApiType type);
		int32 Run();
    };
}

