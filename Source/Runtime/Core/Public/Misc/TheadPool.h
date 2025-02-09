#pragma once
#include "Container.h"
#include "Lock.h"
#include "Platform.h"
#include "HAL/Event.h"
#include "HAL/Thread.h"	

namespace Thunder
{
	/************************ 任务 ************************************/
	enum class ETaskPriority : uint8
	{
	    High,
	    Normal,
	    Low,
	    Num
	};

	class ITask;
	
	/************************ 任务 ************************************/
	//单个线程管理者
	class ThreadProxy
	{
	private:
	    // 控制线程挂起唤醒等
		IEvent* DoWorkEvent = nullptr;

		/** If true, the thread should exit. */
		std::atomic<bool> TimeToDie { false };

		// 任务及其锁，后面改成无锁队列
		TQueue<ITask*> QueuedTask = {};
		SpinLock SyncQueuedTask;

		class ThreadPoolBase* ThreadPoolOwner = nullptr;
		
		IThread* Thread = nullptr;

	
	public:
		ThreadProxy() = default;
		~ThreadProxy()
		{
			if (DoWorkEvent)
			{
				KillThread();
			}
		}
		void Stop() {}
		uint32 Run();
		bool CreateSingleThread(uint32 InStackSize = 0, const String& InThreadName = "");
		bool CreateThreadPool(class ThreadPoolBase* InPool,uint32 InStackSize = 0);
		void PushTask(ITask* InTask)
		{
			TLockGuard<SpinLock> guard(SyncQueuedTask);
			QueuedTask.push(InTask);
			DoWorkEvent->Trigger();
		}
		void WaitForCompletion();
		bool KillThread();
		void ThreadPoolDoWork(ITask* InTask);
	};

	//线程池管理者
	class ThreadPoolProxy
	{
		
	};
	
/************************ 线程池 ************************************/
class CORE_API IThreadPool
{
public:
    virtual bool Create(uint32 InNumQueuedThreads, uint32 StackSize = (32 * 1024), const String& Name = "") = 0;
    virtual void Destroy() = 0;
    virtual void AddQueuedWork( ITask* InQueuedWork, ETaskPriority InQueuedWorkPriority = ETaskPriority::Normal) = 0;
    virtual bool RetractQueuedWork(ITask* InQueuedWork) = 0;
    [[nodiscard]] virtual int32 GetNumThreads() const = 0;
	virtual void WaitForCompletion() = 0;
public:
    IThreadPool() = default;
    virtual	~IThreadPool() = default;
	IThreadPool(IThreadPool const&) = default;
	IThreadPool& operator =(IThreadPool const&) = default;
	IThreadPool(IThreadPool&&) = default;
	IThreadPool& operator=(IThreadPool&&) = default;

public:
    static IThreadPool* Allocate();
	static void ParallelFor(const TArray<ITask*>& InTasks, uint32 InNumThreads);
};

extern CORE_API IThreadPool* GThreadPool;
    
}

