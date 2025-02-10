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

		IThread* Thread = nullptr;
		
		class ThreadPoolBase* ThreadPoolOwner = nullptr;

		LockFreeFIFOListBase<ITask, 8> QueuedTask {}; // Thread 使用

		ITask* CurrentTask = nullptr; // ThreadPoolOwner 使用

	
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
		uint32 GetThreadId() const { return Thread ? Thread->GetThreadID() : 0; }
		void PushTask(ITask* InTask) //不保证下一个执行
		{
			QueuedTask.Push(InTask);
			DoWorkEvent->Trigger();
		}
		void WaitForCompletion(); // 线程结束
		bool KillThread();
	private:
		
		friend class ThreadPoolBase;
		bool CreateWithThreadPool(class ThreadPoolBase* InPool,uint32 InStackSize = 0);
		void SetCurrentWork(ITask* InTask); //当前物理线程下一个执行，名字可以改一下
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
		virtual void ParallelFor(TFunction<void(uint32, uint32)> &&Body, uint32 NumTask, uint32 BundleSize) = 0;
	};
	
	struct BoundingBox
	{
		float Top;
		float Bottom;
		float Left;
		float Right;
	};

	inline bool CullObject(BoundingBox ObjectBounding)
	{
		return true;
	}


extern CORE_API IThreadPool* GThreadPool;
    
}

