
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"

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

		auto* task = new (TMemory::Malloc<FunctionTask>()) FunctionTask(InFunction);
		PushTask(task);
	}
	

	ITask* IScheduler::GetNextQueuedWork()
	{
		ITask* work = nullptr;
		if (!QueuedWork.IsEmpty())
		{
			work = QueuedWork.Pop();
		}
		return work;
	}

	void SingleScheduler::AttachToThread(ThreadProxy* InThreadProxy)
	{
		TAssert(InThreadProxy != nullptr);
		if (Thread == InThreadProxy)
		{
			return;
		}
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
		for (auto* thread : ThreadList)
		{
			if (thread == InThreadProxy)
			{
				return;
			}
		}
		ThreadList.push_back(InThreadProxy);
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
		auto it = std::ranges::find(ThreadList, InThreadProxy);
		if (it != ThreadList.end())
		{
			InThreadProxy->DetachFromScheduler(this);
			ThreadList.erase(it);
		}
	}

	void PooledTaskScheduler::WaitForCompletionAndThreadExit()
	{
		for (const auto Thread : ThreadList)
		{
			Thread->WaitForCompletion();
		}
	}

	uint32 PooledTaskScheduler::GetThreadId(uint32 threadIndex) const
	{
		return ThreadList[threadIndex]->GetThreadId();
	}

	void PooledTaskScheduler::ParallelFor(TFunction<void(uint32, uint32)>& Body, uint32 NumTask, uint32 BundleSize)
	{
		class TaskBundle : public ITask
		{
		public:
			TaskBundle(TFunction<void(uint32, uint32)>* InFunction, uint32 InStart, uint32 InSize)
			: Function(InFunction)
			, Head(InStart)
			, Size(InSize)
			{}

			void DoWork() override
			{
				(*Function)(Head, Size);
			}

		private:
			TFunction<void(uint32, uint32)>* Function;
			uint32 Head;
			uint32 Size;
		};

		for (uint32 taskId = 0; taskId < NumTask; taskId += BundleSize)
		{
			const auto bundleTask = new (TMemory::Malloc<TaskBundle>()) TaskBundle(&Body, taskId, BundleSize);
			PushTask(bundleTask);
		}
	}

	void PooledTaskScheduler::ParallelFor(TFunction<void(uint32, uint32)>&& Body, uint32 NumTask, uint32 BundleSize)
	{
		class TaskBundle : public ITask
		{
		public:
			TaskBundle(const TFunction<void(uint32, uint32)>& InFunction, uint32 InStart, uint32 InSize)
			: Function(InFunction)
			, Head(InStart)
			, Size(InSize)
			{}

			void DoWork() override
			{
				Function(Head, Size);
			}
		private:

			TFunction<void(uint32, uint32)> Function;
			uint32 Head;
			uint32 Size;
		};

		for (uint32 taskId = 0; taskId < NumTask; taskId += BundleSize)
		{
			const auto bundleTask = new (TMemory::Malloc<TaskBundle>()) TaskBundle(Body, taskId, BundleSize);
			PushTask(bundleTask);
		}
	}

	void TaskSchedulerManager::StartUp()
	{
		TAssert(GGameScheduler == nullptr && GRenderScheduler == nullptr && GRHIScheduler == nullptr);

		// todo : destroy threads
		const auto gameThreadProxy = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy(4096, "GameThread");
		const auto renderThreadProxy = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy(4096, "RenderThread");
		const auto rhiThreadProxy = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy(4096, "RHIThread");

		GGameScheduler = new (TMemory::Malloc<SingleScheduler>()) SingleScheduler();
		GGameScheduler->AttachToThread(gameThreadProxy);

		GRenderScheduler = new (TMemory::Malloc<SingleScheduler>()) SingleScheduler();
		GRenderScheduler->AttachToThread(renderThreadProxy);

		GRHIScheduler = new (TMemory::Malloc<SingleScheduler>()) SingleScheduler();
		GRHIScheduler->AttachToThread(rhiThreadProxy);

		InitWorkerThread();
	}

	void TaskSchedulerManager::InitWorkerThread()
	{
		TAssert(GSyncWorkers == nullptr && GAsyncWorkers == nullptr);
		const auto ThreadNum = FPlatformProcess::NumberOfLogicalProcessors();

		const auto syncThreads = new (TMemory::Malloc<ThreadPoolBase>()) ThreadPoolBase(ThreadNum > 3 ? (ThreadNum - 3) : ThreadNum, 96 * 1024, "SyncWorkerThread");
		const auto asyncThreads = new (TMemory::Malloc<ThreadPoolBase>()) ThreadPoolBase(4, 96 * 1024, "AsyncWorkerThread");

		GSyncWorkers = new (TMemory::Malloc<PooledTaskScheduler>()) PooledTaskScheduler();
		syncThreads->AttachToScheduler(GSyncWorkers);

		GAsyncWorkers = new (TMemory::Malloc<PooledTaskScheduler>()) PooledTaskScheduler();
		asyncThreads->AttachToScheduler(GAsyncWorkers);
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
