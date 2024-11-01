
#pragma optimize("", off)
#include "Misc/TheadPool.h"
#include "Misc/AsyncTask.h"
#include "Container.h"
#include "Misc/Lock.h"

namespace Thunder
{
	struct PriorityQueueTask
	{
		ITask* Task;
		ETaskPriority Priority;

		bool operator<(const PriorityQueueTask& Other) const
		{
			return Priority < Other.Priority;
		}
	};

// FQueuedThreadPoolBase
class ThreadPoolBase : public IThreadPool
{
protected:
	/** The work queue to pull from. */
	TPriorityQueue<PriorityQueueTask> QueuedWork;

	/** The thread pool to dole work out to. */
	TArray<ThreadProxy*> QueuedThreads;

	/** All threads in the pool. */
	TArray<ThreadProxy*> AllThreads;

	/** The synchronization object used to protect access to the queued work. */
	SpinLock SynchQueue;

	/** If true, indicates the destruction process has taken place. */
	bool TimeToDie;

public:

	/** Default constructor. */
	ThreadPoolBase()
		: TimeToDie(false)
	{
	}

	/** Virtual destructor (cleans up the synchronization objects). */
	virtual ~ThreadPoolBase() override
	{
		Destroy();
	}

	virtual bool Create(uint32 InNumQueuedThreads, uint32 StackSize, EThreadPriority ThreadPriority, const String& Name) override
	{
		bool bWasSuccessful = true;
		TLockGuard<SpinLock> guard(SynchQueue); //获得临界区，构造时lock，退出作用域时析构unlock

		TAssert(QueuedThreads.empty());
		//QueuedThreads.resize(InNumQueuedThreads);
		while (!QueuedWork.empty())
		{
			QueuedWork.pop();
		}

		uint32 OverrideStackSize = StackSize;

		// Now create each thread and add it to the array
		for (uint32 Count = 0; Count < InNumQueuedThreads && bWasSuccessful == true; Count++)
		{
			// Create a new queued thread
			auto pThread = new ThreadProxy();
			// Now create the thread and add it if ok
			const String ThreadName = "ThreadPoolThread + Count";
			if (pThread->Create(this, StackSize, ThreadPriority) == true)
			{
				QueuedThreads.push_back(pThread);
				AllThreads.push_back(pThread);
			}
			else
			{
				// Failed to fully create so clean up
				bWasSuccessful = false;
				delete pThread;
			}
		}
		// Destroy any created threads if the full set was not successful
		if (bWasSuccessful == false)
		{
			Destroy();
		}

		return bWasSuccessful;
	}

	virtual void Destroy() override final
	{
		{
			TLockGuard<SpinLock> guard(SynchQueue);
			TimeToDie = 1;
			FPlatformMisc::MemoryBarrier();
			// Clean up all queued objects
			while (!QueuedWork.empty())
			{
				const auto WorkItem = QueuedWork.top().Task;
				QueuedWork.pop();
				WorkItem->Abandon();
			}
		}
		// wait for all threads to finish up
		while (true)
		{
			{
				TLockGuard<SpinLock> guard(SynchQueue);
				if (AllThreads.size() == QueuedThreads.size())
				{
					break;
				}
			}
			FPlatformProcess::Sleep(0.0f);
		}
		// Delete all threads
		{
			TLockGuard<SpinLock> guard(SynchQueue);
			// Now tell each thread to die and delete those
			for (int32 Index = 0; Index < AllThreads.size(); Index++)
			{
				AllThreads[Index]->KillThread();
				delete AllThreads[Index];
			}
			TArray<ThreadProxy*> Empty;
			QueuedThreads.swap(Empty);
			AllThreads.swap(Empty);
		}
	}

	int32 GetNumThreads() const override
	{
		return static_cast<int32>(AllThreads.size());
	}

	void AddQueuedWork(ITask* InQueuedWork, ETaskPriority InQueuedWorkPriority) override
	{
		TAssert(InQueuedWork != nullptr);

		//检查线程是否可用确保在执行此操作时没有其他线程可以操作线程池。
		//
		//我们从数组的后面选择一个线程，因为这将是最近使用的线程，因此最有可能具有堆栈等的“热”缓存（类似于Windows IOCP调度策略）。
		//从后面选择也更便宜，因为不需要移动内存。

		ThreadProxy* Thread;

		{
			TLockGuard<SpinLock> guard(SynchQueue);
			const auto AvailableThreadCount = static_cast<int32>(QueuedThreads.size());
			if (AvailableThreadCount == 0)
			{
				//无可用
				QueuedWork.push({InQueuedWork, InQueuedWorkPriority});
				//LOG("QueuedWork Push Task: %d", QueuedWork.size());
				return;
			}

			const int32 ThreadIndex = AvailableThreadCount - 1;

			Thread = QueuedThreads[ThreadIndex];
			QueuedThreads.pop_back(); //移除队尾
			TAssert(QueuedThreads.size() == ThreadIndex);
		}

		Thread->DoWork(InQueuedWork);
	}

	virtual bool RetractQueuedWork(ITask* InQueuedWork) override
	{
		//todo
		return false;
	}

	ITask* ReturnToPoolOrGetNextJob(ThreadProxy* InQueuedThread)
	{
		TAssert(InQueuedThread != nullptr);
		// Check to see if there is any work to be done
		TLockGuard<SpinLock> guard(SynchQueue);
		if (QueuedWork.empty())
		{
			LOG("Thread Push Pack with QueuedWork.empty(), ThreadID: %d, Num of QueuedThread: %d", FPlatformTLS::GetCurrentThreadId(), QueuedThreads.size());
			QueuedThreads.push_back(InQueuedThread);
			return nullptr;
		}

		ITask* Work = QueuedWork.top().Task;
		QueuedWork.pop();
		TAssertf(Work, "!!!");

		if (!Work)
		{
			LOG("Thread Push Pack with Pop empty error, ThreadID: %d, Num of QueuedThread: %d", FPlatformTLS::GetCurrentThreadId(), QueuedThreads.size());
			// There was no work to be done, so add the thread to the pool
			QueuedThreads.push_back(InQueuedThread);
		}
		return Work;
	}

	void WaitForCompletion() override
	{
		bool NeedWait = false;
		while(true)
		{
			if (NeedWait)
			{
				_sleep(5);
				NeedWait = false;
			}
			{
				TLockGuard<SpinLock> guard(SynchQueue);
				if (QueuedWork.empty() && QueuedThreads.size() == AllThreads.size())
				{
					return;
				}
			}
			ThreadProxy* Thread;
			ITask* WorkItem;
			{
				TLockGuard<SpinLock> guard(SynchQueue);
				const auto AvailableThreadCount = static_cast<int32>(QueuedThreads.size());
				if (AvailableThreadCount == 0)
				{
					NeedWait = true;
					continue;
				}
				if (QueuedWork.empty())
				{
					NeedWait = true;
					continue;
				}
				WorkItem = QueuedWork.top().Task;
				QueuedWork.pop();
				TAssert(WorkItem);
				const int32 ThreadIndex = AvailableThreadCount - 1;

				Thread = QueuedThreads[ThreadIndex];
				QueuedThreads.pop_back(); //移除队尾
				TAssert(QueuedThreads.size() == ThreadIndex);
			}
			
			Thread->DoWork(WorkItem);
		}
	}

	void ParallelForInternal(const TArray<ITask*>& InTasks)
	{
		
	}
};

IThreadPool* IThreadPool::Allocate()
{
	return new ThreadPoolBase();
}

IThreadPool* GThreadPool = IThreadPool::Allocate();
IThreadPool* GCacheThreadPool = nullptr;

void IThreadPool::ParallelFor(const TArray<ITask*>& InTasks, EThreadPriority InThreadPriority, uint32 InNumThreads)
{
	if(!GCacheThreadPool)
	{
		GCacheThreadPool = IThreadPool::Allocate();
	}
	GCacheThreadPool->Create(InNumThreads, 96 * 1024, InThreadPriority, "ParallelForThreadPool");

	for(const auto Task : InTasks)
	{
		GCacheThreadPool->AddQueuedWork(Task);
	}
	GCacheThreadPool->WaitForCompletion();
	GCacheThreadPool->Destroy();
}



//todo: 没明白
uint32 ThreadProxy::Run()
{
	while (!TimeToDie.load(std::memory_order_relaxed))
	{
		DoWorkEvent->Wait();

		// no thread pool
		if (!ThreadPoolOwner)
		{
			LOG("No Pool ThreadID: %d", FPlatformTLS::GetCurrentThreadId());
			ITask* LocalQueuedWork = QueuedTask;
			QueuedTask = nullptr;
			if (LocalQueuedWork)
			{
				LocalQueuedWork->DoThreadedWork();
			}
			return 0;
		}

		// thread pool
		LOG("thread pool ThreadID: %d", FPlatformTLS::GetCurrentThreadId());
		ITask* LocalQueuedWork = QueuedTask;
		LOG("QueuedTask set: null, ThreadID: %d", FPlatformTLS::GetCurrentThreadId());
		QueuedTask = nullptr;
		FPlatformMisc::MemoryBarrier();
		TAssert(LocalQueuedWork || TimeToDie.load(std::memory_order_relaxed));
		while (LocalQueuedWork)
		{
			LOG("LocalQueuedWork do task, ThreadID: %d", FPlatformTLS::GetCurrentThreadId());
			LocalQueuedWork->DoThreadedWork();
			LocalQueuedWork = ThreadPoolOwner->ReturnToPoolOrGetNextJob(this);
		}
	}
	
	return 0;
}

bool ThreadProxy::Create(class ThreadPoolBase* InPool,uint32 InStackSize, EThreadPriority ThreadPriority)
{
	ThreadPoolOwner = InPool;
	DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
	Thread = IThread::Create(this, "NULL", InStackSize, ThreadPriority);
	TAssert(Thread);
	return true;
}

// 强制销毁线程可能会导致TLS泄漏等, 所以杀线程都需要等待完成
bool ThreadProxy::KillThread()
{
	// 线程标记
	TimeToDie = true;
	// 触发Event
	DoWorkEvent->Trigger();
	// 等待完成
	Thread->WaitForCompletion();
	// 回收线程同步Event
	FPlatformProcess::ReturnSyncEventToPool(DoWorkEvent);
	DoWorkEvent = nullptr;
	delete Thread;
	return true;
}

void ThreadProxy::DoWork(ITask* InTask)
{
	TAssert(QueuedTask == nullptr && "Can't do more than one task at a time");
	// 告诉线程来任务了
	LOG("QueuedTask set: %llu, ThreadID: %d", reinterpret_cast<uint64>(InTask), FPlatformTLS::GetCurrentThreadId());
	QueuedTask = InTask;
	FPlatformMisc::MemoryBarrier();
	// Tell the thread to wake up and do its job
	DoWorkEvent->Trigger();
}
    

    
}

#pragma optimize("", on)