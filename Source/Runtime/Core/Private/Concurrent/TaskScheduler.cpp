
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

	void PooledTaskScheduler::ParallelFor(TFunction<void(uint32, uint32, uint32)>& Body, uint32 NumTask, uint32 NumThread)
	{
		class TaskBundle : public ITask
		{
		public:
			TaskBundle(TFunction<void(uint32, uint32, uint32)>* InFunction, uint32 InStart, uint32 InSize, uint32 InThreadId) //接受指针并存下来
			: Function(InFunction)
			, Head(InStart)
			, Size(InSize)
			, ThreadId(InThreadId)
			{}

			void DoWork() override
			{
				(*Function)(Head, Size, ThreadId);
			}
		private:

			TFunction<void(uint32, uint32, uint32)>* Function;
			uint32 Head;
			uint32 Size;
			uint32 ThreadId;
		};

		uint32 bundleSize = (NumTask + NumThread - 1) / NumThread;
		for (uint32 i = 0; i < NumTask; i += bundleSize)
		{
			const auto bundleTask = new (TMemory::Malloc<TaskBundle>()) TaskBundle(&Body, i, bundleSize, i/bundleSize);
			PushTask(bundleTask);
		}
	}

	void PooledTaskScheduler::ParallelFor(TFunction<void(uint32, uint32, uint32)>&& Body, uint32 NumTask, uint32 NumThread)
	{
		class TaskBundle : public ITask
		{
		public:
			TaskBundle(const TFunction<void(uint32, uint32, uint32)>& InFunction, uint32 InStart, uint32 InSize, uint32 InThreadId)
			: Function(InFunction)
			, Head(InStart)
			, Size(InSize)
			, ThreadId(InThreadId)
			{}

			void DoWork() override
			{
				Function(Head, Size, ThreadId);
			}
		private:

			TFunction<void(uint32, uint32, uint32)> Function;
			uint32 Head;
			uint32 Size;
			uint32 ThreadId;
		};

		uint32 bundleSize = (NumTask + NumThread - 1) / NumThread;
		for (uint32 i = 0; i < NumTask; i += bundleSize)
		{
			const auto bundleTask = new (TMemory::Malloc<TaskBundle>()) TaskBundle(Body, i, bundleSize, i/bundleSize);
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
