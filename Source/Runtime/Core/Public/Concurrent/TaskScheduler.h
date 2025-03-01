#pragma once
#include "BasicDefinition.h"
#include "Container.h"
#include "Lock.h"
#include "Platform.h"
#include "Task.h"
#include "Container/LockFree.h"

namespace Thunder
{
	class IScheduler
	{
	public:
		virtual ~IScheduler() = default;
		virtual void AttachToThread(class ThreadProxy* InThreadProxy) = 0;
		virtual void DetachFromThread(ThreadProxy* InThreadProxy) = 0;
		virtual void PushTask(ITask* InQueuedWork) = 0;
		ITask* GetNextQueuedWork();
		_NODISCARD_ bool IsEmptyWork() const { return QueuedWork.IsEmpty(); }
		virtual void WaitForCompletionAndThreadExit() = 0;

	protected:
		LockFreeFIFOListBase<ITask, PLATFORM_CACHE_LINE_SIZE> QueuedWork {};
	};

	class SingleScheduler : public IScheduler
	{
	public:
		_NODISCARD_ ThreadProxy* GetThread() const { return Thread; }
		void AttachToThread(ThreadProxy* InThreadProxy) override;
		void DetachFromThread(ThreadProxy* InThreadProxy) override;
		void PushTask(ITask* InQueuedWork) override;
		void WaitForCompletionAndThreadExit() override;
	private:
		ThreadProxy* Thread {};
	};

	class PooledTaskScheduler : public IScheduler
	{
	public:
		_NODISCARD_ int32 GetNumSchedulers() const { return static_cast<int32>(TaskSchedulers.size()); }
		_NODISCARD_ int32 GetNumThreads() const { return static_cast<int32>(ThreadList.size()); }

		void AttachToThread(ThreadProxy* InThreadProxy) override;
		void AddSingleScheduler(SingleScheduler* InSingleScheduler);
		void PushTask(ITask* InQueuedWork)  override;
		void PushTask(int Index, ITask* InQueuedWork) const;
		void DetachFromThread(ThreadProxy* InThreadProxy) override;
		void WaitForCompletionAndThreadExit() override;

		void ParallelFor(TFunction<void(uint32, uint32)>& Body, uint32 NumTask, uint32 BundleSize);
		void ParallelFor(TFunction<void(uint32, uint32)> &&Body, uint32 NumTask, uint32 BundleSize);

	private:
		friend class ThreadProxy;
		TArray<SingleScheduler*> TaskSchedulers {};
		TSet<ThreadProxy*> ThreadList {};
	};

	class TaskSchedulerManager
	{
	public:
		static void StartUp();
		static void InitWorkerThread();
		static void ShutDown();
	};

	CORE_API extern SingleScheduler* GGameScheduler;
	CORE_API extern SingleScheduler* GRenderScheduler;
	CORE_API extern SingleScheduler* GRHIScheduler;
	CORE_API extern PooledTaskScheduler* GSyncWorkers;
	CORE_API extern PooledTaskScheduler* GAsyncWorkers;
}
