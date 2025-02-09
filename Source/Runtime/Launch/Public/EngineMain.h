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
		bool ready = false;
	};

	extern LAUNCH_API SimpleLock* GThunderEngineLock;

	class PhysicsTask : public TaskAllocator //临时放在这
	{
	public:
		int32 Data;
		PhysicsTask(): Data(0)
		{
		}

		PhysicsTask(int32 InPhysicsTaskData, const String& InDebugName = "")
			: TaskAllocator(InDebugName)
			, Data(InPhysicsTaskData)
		{
		}

		void DoWork() override
		{
			LOG("Execute physical calculation with thread: %lu", __threadid());
		}
	};

	class CullTask : public TaskAllocator //临时放在这
	{
	public:
		int32 Data;
		CullTask(): Data(0)
		{
		}

		CullTask(int32 InCullTaskData, const String& InDebugName = "")
			: TaskAllocator(InDebugName)
			, Data(InCullTaskData)
		{
		}

		void DoWork() override
		{
			LOG("Execute clipping calculation with thread: %lu", __threadid());
		}
	};

	class TickTask : public TaskAllocator //临时放在这
	{
	public:
		int32 Data;
		TickTask(): Data(0)
		{
		}

		TickTask(int32 InTickTaskData, const String& InDebugName = "")
			: TaskAllocator(InDebugName)
			, Data(InTickTaskData)
		{
		}

		void DoWork() override
		{
			LOG("Execute tick calculation with thread: %lu", __threadid());
		}
	};
	
	class GameThread //临时放在这
	{
	public:
		friend class TTask<GameThread>;

		int32 FrameData;
		GameThread(): FrameData(0)
		{
		}

		GameThread(int32 InExampleData)
		 : FrameData(InExampleData)
		{
		}

		void DoWork()
		{
			EngineLoop();
		}
	private:
		void EngineLoop();
		void GameMain();
    
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

