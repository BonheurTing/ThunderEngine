#pragma once
#include "Container.h"
#include "Lock.h"
#include "Platform.h"
#include "HAL/Event.h"
#include "HAL/Thread.h"

namespace Thunder
{
	/************************ task ************************************/
	#define SUSPEND_THRESHOLD 50 
	//single thread manager
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
		FORCEINLINE void SetContextId(uint32 InContextId) { ContextId = InContextId; }
		FORCEINLINE uint32 GetContextId() const { return ContextId; }

		//Thread-safe
		//friend class PooledTaskScheduler;
		void AttachToScheduler(class IScheduler* InScheduler);
		void DetachFromScheduler(IScheduler* InScheduler);

		void Suspend() const { DoWorkEvent->Reset(); }
		void Resume() const { DoWorkEvent->Trigger(); }
		uint32 Run();
		void WaitForCompletion(); // Thread termination

	private:
		bool CreatePhysicalThread(uint32 InStackSize = 0, const String& InThreadName = "");
		bool NoWorkToRun();
		
	private:
		IEvent* DoWorkEvent {};
		std::atomic<bool> TimeToDie { false };
		IThread* Thread {};
		ThreadPoolBase* ThreadPoolOwner {};
		TSet<IScheduler*> AttachedSchedulers {};
		SharedLock SchedulersSharedLock;
		uint32 ContextId = 0;
	};

	CORE_API void SetCurrentThread(ThreadProxy* InThread);
	CORE_API ThreadProxy* GetThread();
	CORE_API uint32 GetContextId();

	/************************ thread pool ************************************/

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

		void WaitForCompletion() const; //Wait for all tasks to complete and then terminate the thread (for debug)

	private:
		TArray<ThreadProxy*> Threads {};
	};
}

