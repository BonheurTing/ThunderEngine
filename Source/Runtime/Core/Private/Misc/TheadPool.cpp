
#pragma optimize("", off)
#include "Misc/TheadPool.h"
#include <future>
#include "Misc/Task.h"
#include "Container.h"
#include "Misc/LazySingleton.h"
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


	


	bool ThreadPoolBase::Create(uint32 InNumQueuedThreads, uint32 StackSize)
	{
		bool ret = true;

		TAssert(QueuedThreads.empty());
		while (!QueuedWork.IsEmpty())
		{
			QueuedWork.Pop();
		}
		
		for (uint32 Count = 0; Count < InNumQueuedThreads && ret == true; Count++)
		{
			const auto pThread = new ThreadProxy();
			const String ThreadName = "WorkerThread_" + std::to_string(Count);
			if (pThread->CreateWithThreadPool(this, StackSize))
			{
				TAssert(pThread);
				QueuedThreads.push_back(pThread);
			}
			else
			{
				ret = false;
				delete pThread;
				break;
			}
		}

		if (ret == false)
		{
			Destroy();
		}

		return ret;
	}

	void ThreadPoolBase::Destroy()
	{
		while (!QueuedWork.IsEmpty())
		{
			const auto WorkItem = QueuedWork.Pop();
			WorkItem->Abandon();
		}

		for (const auto& QueuedThread : QueuedThreads)
		{
			QueuedThread->KillThread();
			delete QueuedThread;
		}
		TArray<ThreadProxy*> Empty;
		QueuedThreads.swap(Empty);
	}
	

	void ThreadPoolBase::AddQueuedWork(ITask* InQueuedWork)
	{
		TAssert(InQueuedWork != nullptr);

		QueuedWork.Push(InQueuedWork);

		for(const auto Thread : QueuedThreads)
		{
			Thread->Resume();
		}
	}

	ITask* ThreadPoolBase::GetNextQueuedWork()
	{
		ITask* Work = nullptr;
		if (!QueuedWork.IsEmpty())
		{
			Work = QueuedWork.Pop();
		}
		return Work;
	}

	void ThreadPoolBase::WaitForCompletion()
	{
		for (const auto Thread : QueuedThreads)
		{
			Thread->WaitForCompletion();
		}
	}

	// todo TFunction还是把类型写死了，最好能任意参数类型
	void ThreadPoolBase::ParallelFor(TFunction<void(uint32, uint32)> &&Body, uint32 NumTask, uint32 BundleSize)
	{
		class TaskBundle : public ITask
		{
		public:
			TaskBundle(const TFunction<void(uint32, uint32)> & InFunction, uint32 InStart, uint32 InSize)
			: Function(std::move(InFunction))
			, Head(InStart)
			, Size(InSize)
			{}

			void DoWork() override
			{
				LOG("Execute Parallel Task Bundle");
				Function(Head, Size);
			}
		private:

			TFunction<void(uint32, uint32)> Function;
			uint32 Head;
			uint32 Size;
		};

		for (uint32 i=0; i < NumTask;i+=BundleSize)
		{
			const auto BundleTask = new TaskBundle(std::move(Body), i, BundleSize);
			AddQueuedWork(BundleTask);
		}
	}

	uint32 ThreadProxy::Run()
	{
		ITask* CurrentWork;
		if (ThreadPoolOwner)
		{
			while (!(ThreadPoolOwner->IsIdle() && TimeToDie.load(std::memory_order_relaxed)))
			{
				DoWorkEvent->Wait();
				
				TAssert(QueuedTasks.IsEmpty());

				int32 NumOfFailed = SUSPEND_THRESHOLD;
				while (NumOfFailed > 0)
				{
					CurrentWork = ThreadPoolOwner->GetNextQueuedWork();
					if(CurrentWork)
					{
						CurrentWork->DoWork();
						ThreadPoolOwner->OnCompleted(CurrentWork);
					}
					else
					{
						NumOfFailed--;
					}
				}
			}
		}
		else
		{
			while (!(QueuedTasks.IsEmpty() && TimeToDie.load(std::memory_order_relaxed)))
			{
				DoWorkEvent->Wait();

				int32 NumOfFailed = SUSPEND_THRESHOLD;
				while (NumOfFailed > 0)
				{
					if (!QueuedTasks.IsEmpty())
					{
						CurrentWork = QueuedTasks.Pop();
						CurrentWork->DoWork();
					}
					else
					{
						NumOfFailed--;
					}
				}
			}
		}
		
		return 0;
	}

	bool ThreadProxy::CreateSingleThread(uint32 InStackSize, const String& InThreadName)
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

	bool ThreadProxy::CreateWithThreadPool(class ThreadPoolBase* InPool,uint32 InStackSize)
	{
		ThreadPoolOwner = InPool;
		DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
		Thread = IThread::Create(this, InStackSize);
		TAssert(DoWorkEvent);
		TAssert(Thread);
		return true;
	}

	void ThreadProxy::WaitForCompletion()
	{
		// 线程标记
		TimeToDie.store(true, std::memory_order_relaxed);
		// 触发Event
		DoWorkEvent->Trigger();
		// 等待完成
		Thread->WaitForCompletion();
	}

	bool ThreadProxy::KillThread()
	{
		WaitForCompletion();
		// 回收线程同步Event
		FPlatformProcess::ReturnSyncEventToPool(DoWorkEvent);
		DoWorkEvent = nullptr;
		delete Thread;
		Thread = nullptr;
		return true;
	}
    
}

#pragma optimize("", on)