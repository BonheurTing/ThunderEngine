
#pragma optimize("", off)
#include "Misc/TheadPool.h"
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


	class ThreadPoolBase : public IThreadPool
	{
	protected:
		TPriorityQueue<PriorityQueueTask> QueuedWork {};

		TArray<ThreadProxy*> QueuedThreads {};

		TArray<ThreadProxy*> AllThreads {};
		
		SpinLock SyncQueuedWork; // QueuedWork AllThreads QueuedThreads的锁

		bool TimeToDie { false };


	public:
		ThreadPoolBase() = default;

		~ThreadPoolBase() override
		{
			ThreadPoolBase::Destroy();
		}

		bool Create(uint32 InNumQueuedThreads, uint32 StackSize) override
		{
			bool ret = true;
			TLockGuard<SpinLock> guard(SyncQueuedWork); //获得临界区，构造时lock，退出作用域时析构unlock

			TAssert(QueuedThreads.empty());
			//QueuedThreads.resize(InNumQueuedThreads);
			while (!QueuedWork.empty())
			{
				QueuedWork.pop();
			}
			
			for (uint32 Count = 0; Count < InNumQueuedThreads && ret == true; Count++)
			{
				auto pThread = new ThreadProxy();
				const String ThreadName = "WorkerThread_" + std::to_string(Count);
				if (pThread->CreateWithThreadPool(this, StackSize))
				{
					TAssert(pThread);
					QueuedThreads.push_back(pThread);
					AllThreads.push_back(pThread);
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

		void Destroy() override
		{
			{
				TLockGuard<SpinLock> guard(SyncQueuedWork);
				TimeToDie = true;
				FPlatformMisc::MemoryBarrier();
				while (!QueuedWork.empty())
				{
					const auto WorkItem = QueuedWork.top().Task;
					QueuedWork.pop();
					WorkItem->Abandon();
				}
			}
			while (true)
			{
				{
					TLockGuard<SpinLock> guard(SyncQueuedWork);
					if (AllThreads.size() == QueuedThreads.size())
					{
						break;
					}
				}
				FPlatformProcess::Sleep(0.0f);
			}

			{
				TLockGuard<SpinLock> guard(SyncQueuedWork);
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

		_NODISCARD_ int32 GetNumThreads() const override
		{
			return static_cast<int32>(AllThreads.size());
		}

		void AddQueuedWork(ITask* InQueuedWork, ETaskPriority InQueuedWorkPriority = ETaskPriority::Normal) override
		{
			TAssert(InQueuedWork != nullptr);

			//检查线程是否可用确保在执行此操作时没有其他线程可以操作线程池。
			//
			//我们从数组的后面选择一个线程，因为这将是最近使用的线程，因此最有可能具有堆栈等的“热”缓存（类似于Windows IOCP调度策略）。
			//从后面选择也更便宜，因为不需要移动内存。

			ThreadProxy* Thread;

			{
				TLockGuard<SpinLock> guard(SyncQueuedWork);
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
				TAssert(Thread);
				QueuedThreads.pop_back(); //移除队尾
				
				TAssert(Thread);
				TAssert(static_cast<int32>(QueuedThreads.size()) == ThreadIndex);
			}

			Thread->SetCurrentWork(InQueuedWork);
		}

		bool RetractQueuedWork(ITask* InQueuedWork) override
		{
			return false;
		}

		ITask* ReturnToPoolOrGetNextJob(ThreadProxy* InQueuedThread)
		{
			TAssert(InQueuedThread != nullptr);
			// Check to see if there is any work to be done
			TLockGuard<SpinLock> guard(SyncQueuedWork);
			if (QueuedWork.empty())
			{
				//LOG("Thread Push Pack with QueuedWork.empty(), ThreadID: %d, Num of QueuedThread: %d", FPlatformTLS::GetCurrentThreadId(), QueuedThreads.size());
				TAssert(InQueuedThread);
				QueuedThreads.push_back(InQueuedThread);
				return nullptr;
			}

			ITask* Work = QueuedWork.top().Task;
			QueuedWork.pop();
			TAssertf(Work, "!!!");

			if (!Work)
			{
				//LOG("Thread Push Pack with Pop empty error, ThreadID: %d, Num of QueuedThread: %d", FPlatformTLS::GetCurrentThreadId(), QueuedThreads.size());
				// There was no work to be done, so add the thread to the pool
				TAssert(InQueuedThread);
				auto Num = QueuedThreads.size();
				QueuedThreads.push_back(InQueuedThread);
				TAssert(QueuedThreads[Num]);
			}
			return Work;
		}

		void WaitForCompletion() override //等待所有任务完成, 并不结束线程
		{
			bool NeedWait = false;
			while(true)
			{
				if (NeedWait)
				{
					Sleep(5);
					NeedWait = false;
				}
				{
					TLockGuard<SpinLock> guard(SyncQueuedWork);
					if (QueuedWork.empty() && QueuedThreads.size() == AllThreads.size())
					{
						return;
					}
				}
				ThreadProxy* Thread;
				ITask* WorkItem;
				{
					TLockGuard<SpinLock> guard(SyncQueuedWork);
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
					TAssert(Thread);
					QueuedThreads.pop_back(); //移除队尾
					TAssert(Thread);
					TAssert(QueuedThreads.size() == ThreadIndex);
				}
				
				Thread->SetCurrentWork(WorkItem);
			}
		}

		// todo TFunction还是把类型写死了，最好能任意参数类型
		void ParallelFor(TFunction<void(uint32, uint32)> &&Body, uint32 NumTask, uint32 BundleSize) override
		{
			class TaskBundle
			{
			public:
				friend class TTask<TaskBundle>;

				TFunction<void(uint32, uint32)> Func;
				uint32 Head;
				uint32 Size;

				TaskBundle(const TFunction<void(uint32, uint32)> &InFunc, uint32 InStart, uint32 InSize)
					: Func(InFunc)
					, Head(InStart)
					, Size(InSize)
				{
				}

				void DoWork() const
				{
					LOG("Execute Parallel Task Bundle");
					Func(Head, Size);
				}
			};
			
			
			for (uint32 i=0; i < NumTask;i+=BundleSize)
			{
				const auto BundleTask = new TTask<TaskBundle>(Body, i, BundleSize);
				AddQueuedWork(BundleTask);
			}
		}
	};

	IThreadPool* IThreadPool::Allocate()
	{
		return new ThreadPoolBase();
	}

	uint32 ThreadProxy::Run()
	{
		bool bNoMoreTask = ThreadPoolOwner ? CurrentTask == nullptr : QueuedTask.IsEmpty();
		while (!(bNoMoreTask && TimeToDie.load(std::memory_order_relaxed)))
		{
			DoWorkEvent->Wait();

			if (!ThreadPoolOwner) // single thread
			{
				while (!QueuedTask.IsEmpty())
				{
					ITask* LocalQueuedWork = QueuedTask.Pop();
					LocalQueuedWork->DoWork();
				}
			}
			else
			{
				// thread pool
				TAssert(QueuedTask.IsEmpty());
				ITask* LocalQueuedWork = CurrentTask;
				FPlatformMisc::MemoryBarrier(); //todo 需要知道为什么加
				TAssert(LocalQueuedWork || TimeToDie.load(std::memory_order_relaxed));
				while (LocalQueuedWork)
				{
					//LOG("LocalQueuedWork do task, ThreadID: %d", FPlatformTLS::GetCurrentThreadId());
					LocalQueuedWork->DoWork();
					LocalQueuedWork = ThreadPoolOwner->ReturnToPoolOrGetNextJob(this);
				}
			}

			bNoMoreTask = ThreadPoolOwner ? CurrentTask == nullptr : QueuedTask.IsEmpty();
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
		TimeToDie = true;
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

	void ThreadProxy::SetCurrentWork(ITask* InTask)
	{
		TAssert(QueuedTask.IsEmpty() && CurrentTask == nullptr && "Can't do more than one task at a time");
		CurrentTask = InTask;

		FPlatformMisc::MemoryBarrier(); //todo ?
		DoWorkEvent->Trigger();
	}
    
}

#pragma optimize("", on)