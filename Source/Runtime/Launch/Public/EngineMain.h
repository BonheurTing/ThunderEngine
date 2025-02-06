#pragma once
#include "Launch.export.h"
#include "Misc/AsyncTask.h"

namespace Thunder
{
	struct LAUNCH_API SimpleLock
	{
		std::mutex mtx;
		std::condition_variable cv;
		bool ready = false;
	};

	extern LAUNCH_API SimpleLock* GThunderEngineLock;
	
	class LAUNCH_API GameThreadTask
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
	
	class LAUNCH_API EngineMain
    {
    public:
    	EngineMain(){}

		int32 FastInit();
    	bool RHIInit(EGfxApiType type);
		int32 Run();
    };
}

