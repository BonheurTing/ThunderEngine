#pragma once
#include "Launch.export.h"
#include "Misc/AsyncTask.h"
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
	
	class GameThreadTask //临时放在这
	{
	public:
		friend class FAsyncTask<GameThreadTask>;

		int32 FrameData;
		GameThreadTask(): FrameData(0)
		{
		}

		GameThreadTask(int32 InExampleData)
		 : FrameData(InExampleData)
		{
		}

		void DoWork()
		{
			EngineLoop();
		}
	private:
		void EngineLoop();
	private:
    
	};

	class RenderThreadTask //临时放在这
	{
	public:
		friend class FAsyncTask<RenderThreadTask>;

		int32 FrameData;
		RenderThreadTask(): FrameData(0)
		{
		}

		RenderThreadTask(int32 InExampleData)
		 : FrameData(InExampleData)
		{
		}

		void DoWork()
		{
			LOG("Execute render thread in frame: %d with thread: %lu", FrameData, __threadid());
		}

	private:
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

