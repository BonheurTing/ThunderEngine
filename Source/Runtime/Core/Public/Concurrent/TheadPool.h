#pragma once
#include "Container.h"
#include "Lock.h"
#include "Platform.h"
#include "HAL/Event.h"
#include "HAL/Thread.h"	

namespace Thunder
{
	/************************ 任务 ************************************/
	#define SUSPEND_THRESHOLD 50 
	//单个线程管理者
	class ThreadProxy
	{
	public:
		ThreadProxy(uint32 InStackSize, const String& InThreadName = "")
		{
			CreatePhysicalThread(InStackSize, InThreadName);
		}
		~ThreadProxy();

		void SetThreadPoolOwner(class ThreadPoolBase* InPool) { ThreadPoolOwner = InPool; }
		_NODISCARD_ uint32 GetThreadId() const;
		_NODISCARD_ NameHandle GetThreadName() const;

		//多线程安全
		//friend class PooledTaskScheduler;
		void AttachToScheduler(class IScheduler* InScheduler);
		void DetachFromScheduler(IScheduler* InScheduler);

		//线程控制
		void Suspend() const { DoWorkEvent->Reset(); }
		void Resume() const { DoWorkEvent->Trigger(); }
		uint32 Run();
		void WaitForCompletion(); // 线程结束

	private:
		bool CreatePhysicalThread(uint32 InStackSize = 0, const String& InThreadName = "");
		bool NoWorkToRun();
		
	private:
		IEvent* DoWorkEvent {}; //控制线程挂起唤醒等状态
		std::atomic<bool> TimeToDie { false }; //线程结束标志
		IThread* Thread {}; //实际物理线程
		ThreadPoolBase* ThreadPoolOwner {};
		TSet<IScheduler*> AttachedSchedulers {};
		SharedLock SchedulersSharedLock;
	};
	
	/************************ 线程池 ************************************/

	class ThreadPoolBase
	{
	public:
		ThreadPoolBase(uint32 ThreadsNum, uint32 StackSize, const String& ThreadNamePrefix = "");

		_NODISCARD_ int32 GetNumThreads() const
		{
			return static_cast<int32>(Threads.size());
		}
		_NODISCARD_ ThreadProxy* GetThread(int32 Index) const
		{
			TAssert(Index < static_cast<int32>(Threads.size()));
			return Threads[Index];
		}

		void AttachToScheduler(IScheduler* InScheduler) const;
		void AttachToScheduler(int32 Index, IScheduler* InScheduler) const;

		void WaitForCompletion() const; //等待所有任务完成, 结束线程, debug用

	private:
		TArray<ThreadProxy*> Threads {};
	};
}

