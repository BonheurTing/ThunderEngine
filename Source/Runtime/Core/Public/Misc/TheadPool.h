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
		LockFreeFIFOListBase<ITask, 8> QueuedTask {};

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
		void PushTask(ITask* InTask)
		{
			QueuedTask.Push(InTask);
			DoWorkEvent->Trigger();
		}
		void WaitForCompletion();
		bool KillThread();
	private:
		
		friend class ThreadPoolBase;
		bool CreateWithThreadPool(class ThreadPoolBase* InPool,uint32 InStackSize = 0);
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
    virtual bool Create(uint32 InNumQueuedThreads, uint32 StackSize = (32 * 1024)) = 0;
    virtual void Destroy() = 0;
    virtual void AddQueuedWork( ITask* InQueuedWork, ETaskPriority InQueuedWorkPriority = ETaskPriority::Normal) = 0;
    virtual bool RetractQueuedWork(ITask* InQueuedWork) = 0;
    _NODISCARD_ virtual int32 GetNumThreads() const = 0;
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
	virtual void ParallelFor(TFunction<void(int32, int32)> &Body, uint32 NumTask, uint32 NumThreads, uint32 BundleSize) = 0;
};

extern CORE_API IThreadPool* GThreadPool;
    
}

