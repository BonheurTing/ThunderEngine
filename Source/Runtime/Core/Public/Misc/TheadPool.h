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
	#define SUSPEND_THRESHOLD 50 
	//单个线程管理者
	class ThreadProxy
	{
	private:
	    // 控制线程挂起唤醒等
		IEvent* DoWorkEvent {};

		std::atomic<bool> TimeToDie { false };

		IThread* Thread {};
		
		class ThreadPoolBase* ThreadPoolOwner {};

		//todo PaddingForCacheContention如何设置, 可以看UE的TaskGraphTest.cpp测试用例
		LockFreeFIFOListBase<ITask, 8> QueuedTasks {}; // Thread 使用 
	
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
		void Resume() { DoWorkEvent->Trigger(); }
		uint32 Run();
		bool CreateSingleThread(uint32 InStackSize = 0, const String& InThreadName = "");
		uint32 GetThreadId() const { return Thread ? Thread->GetThreadID() : 0; }
		void PushTask(ITask* InTask) //不保证下一个执行
		{
			QueuedTasks.Push(InTask);
			DoWorkEvent->Trigger();
		}
		void WaitForCompletion(); // 线程结束
		bool KillThread();
	private:
		
		friend class ThreadPoolBase;
		bool CreateWithThreadPool(class ThreadPoolBase* InPool,uint32 InStackSize = 0);
	};
	
	/************************ 线程池 ************************************/

	class ThreadPoolBase
	{
	protected:
		LockFreeFIFOListBase<ITask, PLATFORM_CACHE_LINE_SIZE> QueuedWork {};
		TArray<ThreadProxy*> QueuedThreads {};
	public:
		ThreadPoolBase() = default;

		~ThreadPoolBase()
		{
			Destroy();
		}
		bool Create(uint32 InNumQueuedThreads, uint32 StackSize);
		void Destroy(); //放弃执行任务，结束线程
		_NODISCARD_ int32 GetNumThreads() const
		{
			return static_cast<int32>(QueuedThreads.size());
		}
		_NODISCARD_ bool IsIdle() const
		{
			return QueuedWork.IsEmpty();
		}
		void AddQueuedWork(ITask* InQueuedWork);
		ITask* GetNextQueuedWork();
		void WaitForCompletion(); //等待所有任务完成, 结束线程, debug用
		void ParallelFor(TFunction<void(uint32, uint32)> &&Body, uint32 NumTask, uint32 BundleSize);
	};



	



	/************************ 测试用 ************************************/
	
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
	
}

