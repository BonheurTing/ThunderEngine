#include "Misc/TheadPool.h"
#include "Misc/AsyncTask.h"
#include "Container.h"

namespace Thunder
{

class FCriticalSection;
class ThreadPoolBase : public IThreadPool
{
protected:

	/** The work queue to pull from. */
	//FThreadPoolPriorityQueue QueuedWork;
	
	/** The thread pool to dole work out to. */
	TArray<ThreadProxy*> QueuedThreads;

	/** All threads in the pool. */
	TArray<ThreadProxy*> AllThreads;

	/** The synchronization object used to protect access to the queued work. */
	FCriticalSection* SynchQueue;

	/** If true, indicates the destruction process has taken place. */
	bool TimeToDie;

public:

	/** Default constructor. */
	ThreadPoolBase()
		: SynchQueue(nullptr)
		, TimeToDie(0)
	{ }

	/** Virtual destructor (cleans up the synchronization objects). */
	virtual ~ThreadPoolBase() override
	{
		Destroy();
	}

	virtual bool Create(uint32 InNumQueuedThreads, uint32 StackSize, EThreadPriority ThreadPriority, const tchar* Name) override
	{
		//todo
		return false;
	}

	virtual void Destroy() override final
	{
		//todo
	}

	virtual int32 GetNumThreads() const 
	{
		return AllThreads.size();
	}

	void AddQueuedWork(ITask* InQueuedWork, ETaskPriority InQueuedWorkPriority) override
	{
		//todo
	}

	virtual bool RetractQueuedWork(ITask* InQueuedWork) override
	{
		//todo
		return false;
	}

	ITask* ReturnToPoolOrGetNextJob(ThreadProxy* InQueuedThread)
	{
		//todo
		return nullptr;
	}
};


uint32 ThreadProxy::Run()
{
	//todo
	return 0;
}

bool ThreadProxy::Create(class FQueuedThreadPoolBase* InPool,uint32 InStackSize, EThreadPriority ThreadPriority)
{
	static int32 PoolThreadIndex = 0;

	ThreadPoolOwner = InPool;
	DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
	Thread = IThread::Create(this, "PoolThread", InStackSize, ThreadPriority);
	TAssert(Thread);
	return true;
}

bool ThreadProxy::KillThread()
{
	bool bDidExitOK = true;
	//todo
	return bDidExitOK;
}

void ThreadProxy::DoWork(ITask* InTask)
{
	//todo
}
    

    
}
