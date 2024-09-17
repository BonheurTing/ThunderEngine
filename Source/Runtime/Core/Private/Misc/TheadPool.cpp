
#pragma optimize("", off)
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

IThreadPool* GThreadPool = new ThreadPoolBase();

//todo: 没明白
uint32 ThreadProxy::Run()
{
	// no thread pool
	{
		LOG("ThreadID: %d", FPlatformTLS::GetCurrentThreadId());
		ITask* LocalQueuedWork = QueuedTask;
		QueuedTask = nullptr;
		TAssert(LocalQueuedWork);
		LocalQueuedWork->DoThreadedWork();
	}
	
	/* thread pool
	while (!TimeToDie.load(std::memory_order_relaxed))
	{
		DoWorkEvent->Wait();

		ITask* LocalQueuedWork = QueuedTask;
		QueuedTask = nullptr;
		FPlatformMisc::MemoryBarrier();
		TAssert(LocalQueuedWork || TimeToDie.load(std::memory_order_relaxed));
		while (LocalQueuedWork)
		{
			LocalQueuedWork->DoThreadedWork();
			LocalQueuedWork = ThreadPoolOwner->ReturnToPoolOrGetNextJob(this);
		}
	}*/
	return 0;
}

bool ThreadProxy::Create(class ThreadPoolBase* InPool,uint32 InStackSize, EThreadPriority ThreadPriority)
{
	//ThreadPoolOwner = InPool;
	//DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
	Thread = IThread::Create(this, "NULL", InStackSize, ThreadPriority);
	TAssert(Thread);
	return true;
}

// 强制销毁线程可能会导致TLS泄漏等, 所以杀线程都需要等待完成
bool ThreadProxy::KillThread()
{
	// 线程标记
	//TimeToDie = true;
	// 触发Event
	//DoWorkEvent->Trigger();
	// 等待完成
	Thread->WaitForCompletion();
	// 回收线程同步Event
	//FPlatformProcess::ReturnSyncEventToPool(DoWorkEvent);
	//DoWorkEvent = nullptr;
	delete Thread;
	return true;
}

void ThreadProxy::DoWork(ITask* InTask)
{
	TAssert(QueuedTask == nullptr && "Can't do more than one task at a time");
	// 告诉线程来任务了
	QueuedTask = InTask;
	//FPlatformMisc::MemoryBarrier();
	// 唤醒线程并执行任务
	//DoWorkEvent->Trigger();
	//todo: 怎么doWork的？
}
    

    
}

#pragma optimize("", on)