
#pragma optimize("", off)
#include "Concurrent/TheadPool.h"
#include <future>
#include "Concurrent/Task.h"
#include "Container.h"
#include "Misc/LazySingleton.h"
#include "Concurrent/Lock.h"
#include "Concurrent/TaskScheduler.h"
#include "Misc/TraceProfile.h"

namespace Thunder
{
	thread_local ThreadProxy* GCurrentThread = nullptr;

	void SetCurrentThread(ThreadProxy* InThread)
	{
		GCurrentThread = InThread;
	}

	ThreadProxy* GetThread()
	{
		return GCurrentThread;
	}

	uint32 GetContextId()
	{
		return GetThread()->GetContextId();
	}

	ThreadProxy::~ThreadProxy()
	{
		if (Thread)
		{
			WaitForCompletion();
			FPlatformProcess::ReturnSyncEventToPool(DoWorkEvent);
			DoWorkEvent = nullptr;
			delete Thread;
			Thread = nullptr;
		}
	}

	uint32 ThreadProxy::GetThreadId() const
	{
		return Thread ? Thread->GetThreadID() : 0;
	}

	NameHandle ThreadProxy::GetThreadName() const
	{
		return Thread ? Thread->GetName() : NameHandle();
	}

	void ThreadProxy::AttachToScheduler(IScheduler* InScheduler)
	{
		TAssert(InScheduler != nullptr);
		{
			auto lock = SchedulersSharedLock.Read();
			if (AttachedSchedulers.contains(InScheduler))
			{
				return;
			}
		}
		InScheduler->AttachToThread(this);
		{
			auto lock = SchedulersSharedLock.Write();
			AttachedSchedulers.emplace(InScheduler);
		}
	}

	void ThreadProxy::DetachFromScheduler(IScheduler* InScheduler)
	{
		TAssert(InScheduler != nullptr);
		{
			auto lock = SchedulersSharedLock.Read();
			if (!AttachedSchedulers.contains(InScheduler))
			{
				return;
			}
		}
		InScheduler->DetachFromThread(this);
		{
			auto lock = SchedulersSharedLock.Write();
			AttachedSchedulers.erase(InScheduler);
		}
	}

	uint32 ThreadProxy::Run()
	{
		ThunderTracyCSetThreadName(GetThreadName().c_str())
		SetCurrentThread(this);

		while (!(TimeToDie.load(std::memory_order_acquire) && NoWorkToRun()))
		{
			DoWorkEvent->Wait();

			int32 numOfFailed = SUSPEND_THRESHOLD;
			while (numOfFailed > 0)
			{
				bool bHasWork = false;
				{
					auto lock = SchedulersSharedLock.Read();
					for (const auto scheduler: AttachedSchedulers)
					{
						if (ITask* currentWork = scheduler->GetNextQueuedWork())
						{
							bHasWork = true;
							numOfFailed = SUSPEND_THRESHOLD;
							currentWork->DoWork();
							TMemory::Destroy(currentWork);
						}
					}
				}
				if (!bHasWork)
				{
					numOfFailed--;
				}
			}
		}

		return 0;
	}

	bool ThreadProxy::CreatePhysicalThread(uint32 InStackSize, const String& InThreadName)
	{
		TAssert(ThreadPoolOwner == nullptr);
		TAssert(DoWorkEvent == nullptr);
		TAssert(Thread == nullptr);

		DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
		Thread = IThread::Create(this, InStackSize, InThreadName);

		TAssert(DoWorkEvent);
		TAssert(Thread);
		return true;
	}

	bool ThreadProxy::NoWorkToRun()
	{
		{
			auto lock = SchedulersSharedLock.Read();
			for (const auto Scheduler : AttachedSchedulers)
			{
				if (!Scheduler->IsEmptyWork())
				{
					return false;
				}
			}
		}
		return true;
	}

	void ThreadProxy::WaitForCompletion()
	{
		TimeToDie.store(true, std::memory_order_release);
		DoWorkEvent->Trigger();
		Thread->WaitForCompletion();
	}

	ThreadPoolBase::ThreadPoolBase(uint32 ThreadsNum, uint32 StackSize, const String& ThreadNamePrefix)
	{
		for (uint32 threadIndex = 0; threadIndex < ThreadsNum; threadIndex++)
		{
			if (auto thread = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy(StackSize, ThreadNamePrefix + "_" + std::to_string(threadIndex)))
			{
				thread->SetThreadPoolOwner(this);
				thread->SetContextId(threadIndex);
				Threads.push_back(thread);
			}
			else
			{
				TAssert(false);
				break;
			}
		}
		ThreadManager::Get().AddThreadPool(this);
	}

	void ThreadPoolBase::AttachToScheduler(IScheduler* InScheduler) const
	{
		for (const auto Thread : Threads)
		{
			Thread->AttachToScheduler(InScheduler);
		}
	}

	void ThreadPoolBase::AttachToScheduler(int32 Index, IScheduler* InScheduler) const
	{
		TAssert(Index < static_cast<int32>(Threads.size()));
		Threads[Index]->AttachToScheduler(InScheduler);
	}

	void ThreadPoolBase::WaitForCompletion() const
	{
		for (const auto Thread : Threads)
		{
			Thread->WaitForCompletion();
		}
	}
    
}
