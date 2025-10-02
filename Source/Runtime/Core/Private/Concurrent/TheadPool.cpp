
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

	ThreadProxy::~ThreadProxy()
	{
		if (Thread)
		{
			WaitForCompletion();
			// 回收线程同步Event
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
			SchedulersSharedLock.Read();
			if (AttachedSchedulers.contains(InScheduler))
			{
				return;
			}
		}
		InScheduler->AttachToThread(this);
		{
			SchedulersSharedLock.Write();
			AttachedSchedulers.emplace(InScheduler);
		}
	}

	void ThreadProxy::DetachFromScheduler(IScheduler* InScheduler)
	{
		TAssert(InScheduler != nullptr);
		{
			SchedulersSharedLock.Read();
			if (!AttachedSchedulers.contains(InScheduler))
			{
				return;
			}
		}
		InScheduler->DetachFromThread(this);
		{
			SchedulersSharedLock.Write();
			AttachedSchedulers.erase(InScheduler);
		}
	}

	uint32 ThreadProxy::Run()
	{
		ThunderTracyCSetThreadName(GetThreadName().c_str());

		while (!(TimeToDie.load(std::memory_order_acquire) && NoWorkToRun()))
		{
			DoWorkEvent->Wait();

			int32 NumOfFailed = SUSPEND_THRESHOLD;
			while (NumOfFailed > 0)
			{
				bool bHasWork = false;
				{
					SchedulersSharedLock.Read();
					for (const auto Scheduler: AttachedSchedulers)
					{
						if (ITask* CurrentWork = Scheduler->GetNextQueuedWork())
						{
							LOG(CurrentWork->GetName().c_str());
							bHasWork = true;
							NumOfFailed = SUSPEND_THRESHOLD;
							CurrentWork->DoWork();
							//TMemory::Destroy(CurrentWork);
						}
					}
				}
				if (!bHasWork)
				{
					NumOfFailed--;
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
			SchedulersSharedLock.Read();
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
		// 线程标记
		TimeToDie.store(true, std::memory_order_release);
		// 触发Event
		DoWorkEvent->Trigger();
		// 等待完成
		Thread->WaitForCompletion();
	}

	ThreadPoolBase::ThreadPoolBase(uint32 ThreadsNum, uint32 StackSize, const String& ThreadNamePrefix)
	{
		for (uint32 Count = 0; Count < ThreadsNum; Count++)
		{
			if (auto pThread = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy(StackSize, ThreadNamePrefix + "_" + std::to_string(Count)))
			{
				pThread->SetThreadPoolOwner(this);
				Threads.push_back(pThread);
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
