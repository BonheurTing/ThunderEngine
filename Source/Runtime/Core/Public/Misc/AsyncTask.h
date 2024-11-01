#pragma once
#include <functional>

#include "Misc/TheadPool.h"
#include "Templates/Function.h"


namespace Thunder
{

// CORE_API void AsyncTask
class RawTask : public ITask
{
public:
	RawTask(TFunction<void()>&& InFunction)
		: Function(MoveTemp(InFunction))
	{}

	void DoThreadedWork() override { Function(); }
private:

	TFunction<void()> Function;
};
	

/**
	FAsyncTask - template task for jobs queued to thread pools

	Sample code:

	class ExampleAsyncTask : public FNonAbandonableTask
	{
		friend class FAsyncTask<ExampleAsyncTask>;

		int32 ExampleData;

		ExampleAsyncTask(int32 InExampleData)
		 : ExampleData(InExampleData)
		{
		}

		void DoWork()
		{
			... do the work here
		}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(ExampleAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
		}
	};

	void Example()
	{

		//start an example job

		FAsyncTask<ExampleAsyncTask>* MyTask = new FAsyncTask<ExampleAsyncTask>( 5 );
		MyTask->StartBackgroundTask();

		//--or --

		MyTask->StartSynchronousTask();

		//to just do it now on this thread
		//Check if the task is done :

		if (MyTask->IsDone())
		{
		}

		//Spinning on IsDone is not acceptable( see EnsureCompletion ), but it is ok to check once a frame.
		//Ensure the task is done, doing the task on the current thread if it has not been started, waiting until completion in all cases.

		MyTask->EnsureCompletion();
		delete Task;
	}
**/
template<typename TTask>
class FAsyncTask
	: public ITask
{
	/** User job embedded in this task */ 
	TTask Task;

public:
	FAsyncTask()
		: Task()
	{
		// Cache the StatId to remain backward compatible with TTask that declare GetStatId as non-const.
		//Init(Task.GetStatId());
	}

	/** Forwarding constructor. */
	template <typename Arg0Type, typename... ArgTypes>
	FAsyncTask(Arg0Type&& Arg0, ArgTypes&&... Args)
		: Task(std::forward<Arg0Type>(Arg0), std::forward<ArgTypes>(Args)...)
	{
		// Cache the StatId to remain backward compatible with TTask that declare GetStatId as non-const.
		/*Init(Task.GetStatId());*/
	}
	void StartTask() { DoThreadedWork();}
	//void EnsureCompletion() {}
	void DoThreadedWork() override { Task.DoWork(); }
	void Abandon() override {}

private:

	/** If we aren't doing the work synchronously, this will hold the completion event */
	//IEvent*				DoneEvent = nullptr;
	/** Pool we are queued into, maintained by the calling thread */
	//IThreadPool*	QueuedPool = nullptr;
	/** Current priority */
	//ETaskPriority Priority = ETaskPriority::Normal;
	/** Approximation of the peak memory (in bytes) this task could require during it's execution. */
	//int64 RequiredMemory = -1;
};

}
