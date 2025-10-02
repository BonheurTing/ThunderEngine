
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "Templates/FunctionMy.h"

namespace Thunder
{
	SingleScheduler* GGameScheduler {};
	SingleScheduler* GRenderScheduler {};
	SingleScheduler* GRHIScheduler {};
	PooledTaskScheduler* GSyncWorkers {};
	PooledTaskScheduler* GAsyncWorkers {};

	void IScheduler::PushTask(const TFunction<void()>& InFunction)
	{
		class FunctionTask : public ITask
		{
		public:
			FunctionTask(const TFunction<void()>& InFunc) : Function(InFunc) {}

			void DoWork() override 
			{ 
				Function(); 
			}

		private:
			TFunction<void()> Function;
		};

		auto* Task = new (TMemory::Malloc<FunctionTask>()) FunctionTask(InFunction);
		PushTask(Task);
	}
	

	ITask* IScheduler::GetNextQueuedWork()
	{
		ITask* Work = nullptr;
		if (!QueuedWork.IsEmpty())
		{
			Work = QueuedWork.Pop();
		}
		return Work;
	}

	void SingleScheduler::AttachToThread(ThreadProxy* InThreadProxy)
	{
		TAssert(InThreadProxy != nullptr);
		if (Thread == InThreadProxy)
		{
			return;
		}
		//todo 保证线程安全
		if (Thread != nullptr && Thread != InThreadProxy)
		{
			DetachFromThread(Thread);
		}
		TAssert(Thread == nullptr);
		Thread = InThreadProxy;
		Thread->AttachToScheduler(this);
	}

	void SingleScheduler::DetachFromThread(ThreadProxy* InThreadProxy)
	{
		TAssert(InThreadProxy != nullptr);
		if (Thread == InThreadProxy)
		{
			Thread->DetachFromScheduler(this);
			Thread = nullptr;
		}
	}

	void SingleScheduler::PushTask(ITask* InQueuedWork)
	{
		QueuedWork.Push(InQueuedWork);
		Thread->Resume();
	}

	void SingleScheduler::PushTask(const TFunction<void()>& InFunction)
	{
		IScheduler::PushTask(InFunction);
	}

	void SingleScheduler::WaitForCompletionAndThreadExit()
	{
		if (Thread)
		{
			Thread->WaitForCompletion();
		}
	}

	void PooledTaskScheduler::AttachToThread(ThreadProxy* InThreadProxy)
	{
		TAssert(InThreadProxy != nullptr);
		if (ThreadList.contains(InThreadProxy))
		{
			return;
		}
		ThreadList.emplace(InThreadProxy);
		InThreadProxy->AttachToScheduler(this);
	}

	void PooledTaskScheduler::AddSingleScheduler(SingleScheduler* InSingleScheduler)
	{
		TaskSchedulers.push_back(InSingleScheduler);
		AttachToThread(InSingleScheduler->GetThread());
	}

	void PooledTaskScheduler::PushTask(ITask* InQueuedWork)
	{
		QueuedWork.Push(InQueuedWork);

		for (const auto Thread : ThreadList)
		{
			Thread->Resume();
		}
	}

	void PooledTaskScheduler::PushTask(const TFunction<void()>& InFunction)
	{
		IScheduler::PushTask(InFunction);
	}
	
	void PooledTaskScheduler::PushTask(int Index, ITask* InQueuedWork) const
	{
		TAssert(Index < static_cast<int>(TaskSchedulers.size()));
		TaskSchedulers[Index]->PushTask(InQueuedWork);
	}

	void PooledTaskScheduler::DetachFromThread(ThreadProxy* InThreadProxy)
	{
		TAssert(InThreadProxy != nullptr);
		if (ThreadList.contains(InThreadProxy))
		{
			InThreadProxy->DetachFromScheduler(this);
			ThreadList.erase(InThreadProxy);
		}
	}

	void PooledTaskScheduler::WaitForCompletionAndThreadExit()
	{
		for (const auto Thread : ThreadList)
		{
			Thread->WaitForCompletion();
		}
	}

	void PooledTaskScheduler::ParallelFor(TFunction<void(uint32, uint32, uint32)>& Body, uint32 NumTask, uint32 BundleSize)
	{
		class TaskBundle : public ITask
		{
		public:
			TaskBundle(TFunction<void(uint32, uint32, uint32)>* InFunction, uint32 InStart, uint32 InSize, uint32 InBundleId) //接受指针并存下来
			: Function(InFunction)
			, Head(InStart)
			, Size(InSize)
			, BundleId(InBundleId)
			{}

			void DoWork() override
			{
				LOG("Execute Parallel Task Bundle");
				(*Function)(Head, Size, BundleId);
			}
		private:

			TFunction<void(uint32, uint32, uint32)>* Function; //存TFunction指针，不用构造，只需8字节
			uint32 Head;
			uint32 Size;
			uint32 BundleId;
		};

		for (uint32 i = 0; i < NumTask; i += BundleSize)
		{
			//TaskBundle构造需要TFunction指针，因此传入地址；对于为什么ParallelFor用&接完这里还要&，想象接的是String& Body，TaskBundle如果不加&，就是把String类型给String*类型，无法匹配构造函数
			const auto BundleTask = new (TMemory::Malloc<TaskBundle>()) TaskBundle(&Body, i, BundleSize, i/BundleSize);
			PushTask(BundleTask);
		}
	}

	void PooledTaskScheduler::ParallelFor(TFunction<void(uint32, uint32, uint32)>&& Body, uint32 NumTask, uint32 BundleSize)
	{
		class TaskBundle : public ITask
		{
		public:
			TaskBundle(const TFunction<void(uint32, uint32, uint32)>& InFunction, uint32 InStart, uint32 InSize, uint32 InThreadId) //接引用
			: Function(InFunction) //变量不是指针，免不了一次构造，注意TFunction的大小，是func一个指针大小(8字节)+capture data的大小；
			//对于TestMultiThread.cpp的例子，capture data是一个lambda对象，其中capture了两个vector的引用，即两个指针，大小是16字节；那么一共24字节，对于capture数据较小的情况可以用这个方式
			, Head(InStart)
			, Size(InSize)
			, ThreadId(InThreadId)
			{}

			void DoWork() override
			{
				LOG("Execute Parallel Task Bundle");
				Function(Head, Size, ThreadId);
			}
		private:

			TFunction<void(uint32, uint32, uint32)> Function; //存Function值而不是指针，免不了构造
			uint32 Head;
			uint32 Size;
			uint32 ThreadId;
		};

		for (uint32 i = 0; i < NumTask; i += BundleSize)
		{
			//右值，此处是消亡值，直接传给构造函数
			const auto BundleTask = new (TMemory::Malloc<TaskBundle>()) TaskBundle(Body, i, BundleSize, i/BundleSize);
			PushTask(BundleTask);
		}
	}

	void TaskSchedulerManager::StartUp()
	{
		TAssert(GGameScheduler == nullptr && GRenderScheduler == nullptr && GRHIScheduler == nullptr);

		//todo 线程释放
		const auto GameThreadProxy = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy(4096, "GameThread");
		const auto RenderThreadProxy = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy(4096, "RenderThread");
		const auto RHIThreadProxy = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy(4096, "RHIThread");

		GGameScheduler = new (TMemory::Malloc<SingleScheduler>()) SingleScheduler();
		GGameScheduler->AttachToThread(GameThreadProxy);

		GRenderScheduler = new (TMemory::Malloc<SingleScheduler>()) SingleScheduler();
		GRenderScheduler->AttachToThread(RenderThreadProxy);

		GRHIScheduler = new (TMemory::Malloc<SingleScheduler>()) SingleScheduler();
		GRHIScheduler->AttachToThread(RHIThreadProxy);
	}

	void TaskSchedulerManager::InitWorkerThread()
	{
		TAssert(GSyncWorkers == nullptr && GAsyncWorkers == nullptr);
		const auto ThreadNum = FPlatformProcess::NumberOfLogicalProcessors();

		const auto SyncThreads = new (TMemory::Malloc<ThreadPoolBase>()) ThreadPoolBase(ThreadNum > 3 ? (ThreadNum - 3) : ThreadNum, 96 * 1024, "SyncWorkerThread");
		const auto AsyncThreads = new (TMemory::Malloc<ThreadPoolBase>()) ThreadPoolBase(4, 96 * 1024, "AsyncWorkerThread");

		GSyncWorkers = new (TMemory::Malloc<PooledTaskScheduler>()) PooledTaskScheduler();
		SyncThreads->AttachToScheduler(GSyncWorkers);

		GAsyncWorkers = new (TMemory::Malloc<PooledTaskScheduler>()) PooledTaskScheduler();
		AsyncThreads->AttachToScheduler(GAsyncWorkers);
	}

	void TaskSchedulerManager::ShutDown()
	{
		TMemory::Destroy(GGameScheduler);
		TMemory::Destroy(GRenderScheduler);
		TMemory::Destroy(GRHIScheduler);

		TMemory::Destroy(GSyncWorkers);
		TMemory::Destroy(GAsyncWorkers);
	}
}
