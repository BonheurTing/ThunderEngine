#pragma optimize("", off)
#include "Assertion.h"
#include "HAL/Thread.h"
#include "Misc/FTaskGraphInterface.h"

namespace Thunder
{
    class TaskGraphImplementation;
    struct FWorkerThread;
    static FTaskGraphInterface* TaskGraphImplementationSingleton = NULL;

	FORCEINLINE void TestRandomizedThreads()
	{
	}

    // FTaskThreadBase : FRunnable; FRunnable是可以任意线程上“运行”的对象，看后面是否需要抽象出来，暂时不用管
    class FTaskThreadBase
    {
    public:
        FTaskThreadBase()
                : ThreadId(ENamedThreads::AnyThread)
                , PerThreadIdTLSSlot(0xffffffff)
                , OwnerWorker(nullptr)
        {
            NewTasks.resize(128);
        }

        void Setup(ENamedThreads::Type InThreadId, uint32 InPerThreadIDTLSSlot, FWorkerThread* InOwnerWorker)
        {
            ThreadId = InThreadId;
            TAssert(ThreadId >= 0);
            PerThreadIdTLSSlot = InPerThreadIDTLSSlot;
            OwnerWorker = InOwnerWorker;
        }

    	void InitializeForCurrentThread()
        {
        	FPlatformTLS::SetTlsValue(PerThreadIdTLSSlot,OwnerWorker);
        }

    	ENamedThreads::Type GetThreadId() const
        {
        	TAssert(OwnerWorker); // make sure we are started up
        	return ThreadId;
        }

    	virtual void ProcessTasksUntilQuit(int32 QueueIndex) = 0;

		/** Used for named threads to start processing tasks until the thread is idle and RequestQuit has been called. **/
		virtual uint64 ProcessTasksUntilIdle(int32 QueueIndex)
		{
			TAssert(0);
			return 0;
		}

    	virtual void WakeUp(int32 QueueIndex = 0)
        {
        }

    	virtual void EnqueueFromThisThread(int32 QueueIndex, FBaseGraphTask* Task)
        {
	        TAssert(0);
        }

    	virtual void EnqueueFromOtherThread(int32 QueueIndex, FBaseGraphTask* Task)
        {
        	TAssert(0);
        }

		//
		virtual void Tick()
    	{
    		ProcessTasksUntilIdle(0);
    	}

		// FRunnable API
		virtual bool Init()
    	{
    		InitializeForCurrentThread();
    		return true;
    	}

    	virtual uint32 Run()
        {
        	TAssert(OwnerWorker); // make sure we are started up
        	ProcessTasksUntilQuit(0);
        	return 0;
        }

    protected:
        ENamedThreads::Type ThreadId;
        uint32 PerThreadIdTLSSlot;
        TArray<FBaseGraphTask*> NewTasks;
        FWorkerThread* OwnerWorker;
    };

    // FNamedTaskThread
    class NamedTaskThread : public FTaskThreadBase
    {
    public:
    	void ProcessTasksUntilQuit(int32 QueueIndex) {}
    };

    // FTaskThreadAnyThread
    class FTaskThreadAnyThread : public FTaskThreadBase
    {
    public:
        FTaskThreadAnyThread(int32 InPriorityIndex)
        : PriorityIndex(InPriorityIndex)
        {
        }

    	void ProcessTasksUntilQuit(int32 QueueIndex) override
        {
        	TAssert(!QueueIndex);
        	const bool bIsMultiThread = FPlatformProcess::SupportsMultithreading();
        	do
        	{
        		ProcessTasks();			
        	} while (!Queue.QuitForShutdown && bIsMultiThread); // @Hack - quit now when running with only one thread.
        }

    	void WakeUp(int32 QueueIndex = 0) final override
        {
        	TAssert(false);
        }
    	
    private:
    	uint64 ProcessTasks()
		{
			bool bCountAsStall = true;
			uint64 ProcessedTasks = 0;
			TAssert(++Queue.RecursionGuard == 1);
			while (1)
			{
				FBaseGraphTask* Task = FindWork();
				if (!Task)
				{
					TestRandomizedThreads();
					const bool bIsMultithread = FPlatformProcess::SupportsMultithreading();
					if (bIsMultithread)
					{
						Queue.StallRestartEvent->Wait(0xffffffff, bCountAsStall);
					}
					if (Queue.QuitForShutdown || !bIsMultithread)
					{
						break;
					}
					TestRandomizedThreads();
					continue;
				}
				TestRandomizedThreads();
				Task->Execute(NewTasks, ENamedThreads::Type(ThreadId), true);
				ProcessedTasks++;
				TestRandomizedThreads();
			}
			TAssert(!--Queue.RecursionGuard);
			return ProcessedTasks;
		}

    	struct CRITICAL_SECTION { void* Opaque1[1]; long Opaque2[2]; void* Opaque3[3]; };

    	/** Grouping of the data for an individual queue. **/
    	struct FThreadTaskQueue
    	{
    		/** Event that this thread blocks on when it runs out of work. **/
    		IEvent* StallRestartEvent;
    		/** We need to disallow reentry of the processing loop **/
    		uint32 RecursionGuard;
    		/** Indicates we executed a return task, so break out of the processing loop. **/
    		bool QuitForShutdown;
    		/** Should we stall for tuning? **/
    		bool bStallForTuning;
    		Mutex StallForTuning;

    		FThreadTaskQueue()
				: StallRestartEvent(FPlatformProcess::GetSyncEventFromPool(false))
				, RecursionGuard(0)
				, QuitForShutdown(false)
				, bStallForTuning(false)
    		{

    		}
    		~FThreadTaskQueue()
    		{
    			FPlatformProcess::ReturnSyncEventToPool(StallRestartEvent);
    			StallRestartEvent = nullptr;
    		}
    	};

    	FBaseGraphTask* FindWork();

    	/** Array of queues, only the first one is used for unnamed threads. **/
    	FThreadTaskQueue Queue;
    	
        int32 PriorityIndex;
    };

    struct FWorkerThread
    {
        //Worker对象，负责调度和执行任务
        FTaskThreadBase*	TaskGraphWorker;
        //线程对象
        IThread*	RunnableThread;

        FWorkerThread()
            : TaskGraphWorker(nullptr)
            , RunnableThread(nullptr)
        {
        }
    };
    
    // FTaskGraphImplementation 类似于manager
    class TaskGraphImplementation final : public FTaskGraphInterface
    {
    public:
    	/**----------与系统生命周期和单例相关的API-----------*/
        
        static TaskGraphImplementation& Get()
        {
            TAssert(TaskGraphImplementationSingleton);
            return *static_cast<TaskGraphImplementation*>(TaskGraphImplementationSingleton);
        }

        // 设置单例并构造
        TaskGraphImplementation(int32)
        {
            NumThreads = 6; //假设
        	NumTaskThreadSets = 1;
            NumNamedThreads = 2;
        	NumTaskThreadsPerSet = 1;
        	bCreatedHiPriorityThreads = false;
        	bCreatedBackgroundPriorityThreads = false;

            PerThreadIdTLSSlot = FPlatformTLS::AllocTlsSlot();
            for (int32 ThreadIndex = 0; ThreadIndex < NumThreads; ThreadIndex++)
            {
                if (ThreadIndex >= NumNamedThreads)
                {
                    WorkerThreads[ThreadIndex].TaskGraphWorker = new FTaskThreadAnyThread(1); //priority index
                }
                else
                {
                    WorkerThreads[ThreadIndex].TaskGraphWorker = new NamedTaskThread();
                }
                WorkerThreads[ThreadIndex].TaskGraphWorker->Setup(ENamedThreads::Type(ThreadIndex), PerThreadIdTLSSlot, &WorkerThreads[ThreadIndex]);
            }
        }

        ~TaskGraphImplementation() override
        {
            for (int32 ThreadIndex = 0; ThreadIndex < NumThreads; ThreadIndex++)
            {
                WorkerThreads[ThreadIndex].RunnableThread->WaitForCompletion();
                delete WorkerThreads[ThreadIndex].RunnableThread;
                WorkerThreads[ThreadIndex].RunnableThread = nullptr;
            }
            TaskGraphImplementationSingleton = nullptr;
            FPlatformTLS::FreeTlsSlot(PerThreadIdTLSSlot);
        }

    	void PushQueuedTask(FBaseGraphTask* Task, bool bWakeUpWorker, ENamedThreads::Type ThreadToExecuteOn, ENamedThreads::Type InCurrentThreadIfKnown = ENamedThreads::AnyThread) override
		{
			if (ENamedThreads::GetThreadIndex(ThreadToExecuteOn) == ENamedThreads::AnyThread)
			{
				if (FPlatformProcess::SupportsMultithreading())
				{
					//todo: Task Priority
					uint32 PriIndex = 0;
					int32 Priority = 0;
					{
						int32 IndexToStart = IncomingAnyThreadTasks[Priority].Push(Task, PriIndex);
						if (IndexToStart >= 0)
						{
							StartTaskThread(Priority, IndexToStart);
						}
					}
					return;
				}
				else
				{
					ThreadToExecuteOn = ENamedThreads::GameThread;
				}
			}
			ENamedThreads::Type CurrentThreadIfKnown;
			if (ENamedThreads::GetThreadIndex(InCurrentThreadIfKnown) == ENamedThreads::AnyThread)
			{
				CurrentThreadIfKnown = GetCurrentThread();
			}
			else
			{
				CurrentThreadIfKnown = ENamedThreads::GetThreadIndex(InCurrentThreadIfKnown);
				TAssert(CurrentThreadIfKnown == ENamedThreads::GetThreadIndex(GetCurrentThread()));
			}
			{
				ThreadToExecuteOn = ENamedThreads::GetThreadIndex(ThreadToExecuteOn);
  				FTaskThreadBase* Target = &ThreadFetch(ThreadToExecuteOn);
				if (ThreadToExecuteOn == ENamedThreads::GetThreadIndex(CurrentThreadIfKnown))
				{
					Target->EnqueueFromThisThread(0, Task);
				}
				else
				{
					Target->EnqueueFromOtherThread(0, Task);
				}
			}
		}

    	/**----------外部线程的API-----------*/


    	void AttachToThread(ENamedThreads::Type CurrentThread) final override
        {
        	CurrentThread = ENamedThreads::GetThreadIndex(CurrentThread);
        	TAssert(CurrentThread >= 0 && CurrentThread < NumNamedThreads);
        	ThreadFetch(CurrentThread).InitializeForCurrentThread();
        }

    	void WaitUntilTasksComplete(const FGraphEventArray& Tasks, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread) final override
		{
			ENamedThreads::Type CurrentThread = CurrentThreadIfKnown;
			const auto DummyCurTh = GetCurrentThread();
			if (ENamedThreads::GetThreadIndex(CurrentThreadIfKnown) == ENamedThreads::AnyThread)
			{
				CurrentThreadIfKnown = ENamedThreads::GetThreadIndex(DummyCurTh);
			}
			else
			{
				CurrentThreadIfKnown = ENamedThreads::GetThreadIndex(CurrentThreadIfKnown);
				TAssert(CurrentThreadIfKnown == ENamedThreads::GetThreadIndex(DummyCurTh));
				// we don't modify CurrentThread here because it might be a local queue
			}

			if (CurrentThreadIfKnown != ENamedThreads::AnyThread && CurrentThreadIfKnown < NumNamedThreads)
			{
				TAssert(false);
			}
			else
			{
				if (!FPlatformProcess::SupportsMultithreading())
				{
					bool bAnyPending = false;
					for (int32 Index = 0; Index < Tasks.size(); Index++)
					{
						FGraphEvent* Task = Tasks[Index].Get();
						if (Task && !Task->IsComplete())
						{
							bAnyPending = true;
							break;
						}
					}
					if (!bAnyPending)
					{
						return;
					}
					LOG("Recursive waits are not allowed in single threaded mode.");
				}
			
				// We will just stall this thread on an event while we wait
				FScopedEvent Event;
				TriggerEventWhenTasksComplete(Event.Get(), Tasks, CurrentThreadIfKnown);
			}
		}

    	void TriggerEventWhenTasksComplete(IEvent* InEvent, const FGraphEventArray& Tasks, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread, ENamedThreads::Type TriggerThread = ENamedThreads::AnyThread) final override
        {
        	TAssert(InEvent);
        	bool bAnyPending = true;
        	if (Tasks.size() < 8) // don't bother to check for completion if there are lots of prereqs...too expensive to check
        	{
        		bAnyPending = false;
        		for (int32 Index = 0; Index < Tasks.size(); Index++)
        		{
        			FGraphEvent* Task = Tasks[Index].Get();
        			if (Task && !Task->IsComplete())
        			{
        				bAnyPending = true;
        				break;
        			}
        		}
        	}
        	if (!bAnyPending)
        	{
        		TestRandomizedThreads();
        		InEvent->Trigger();
        		return;
        	}
        	//todo: bonheur
        	//TGraphTask<FTriggerEventGraphTask>::CreateTask(&Tasks, CurrentThreadIfKnown).ConstructAndDispatchWhenReady(InEvent, TriggerThread);
        }

    	/**----------调度函数-----------*/

    	void StartTaskThread(int32 Priority, int32 IndexToStart)
        {
        	ENamedThreads::Type ThreadToWake = ENamedThreads::Type(IndexToStart + Priority * NumTaskThreadsPerSet + NumNamedThreads);
        	((FTaskThreadAnyThread&)ThreadFetch(ThreadToWake)).WakeUp();
        }

    	FBaseGraphTask* FindWork(ENamedThreads::Type ThreadInNeed) override
        {
        	int32 MyIndex = int32((uint32(ThreadInNeed) - NumNamedThreads) % NumTaskThreadsPerSet);
        	int32 Priority = int32((uint32(ThreadInNeed) - NumNamedThreads) / NumTaskThreadsPerSet);
        	TAssert(MyIndex >= 0 && MyIndex < 3 &&
				Priority >= 0 && Priority < 1);

        	return IncomingAnyThreadTasks[Priority].Pop(MyIndex, true);
        }

    	
    private:
    	//内部函数：验证索引并返回相应TaskThread
    	FTaskThreadBase& ThreadFetch(int32 Index)
    	{
    		TAssert(Index >= 0 && Index < NumThreads);
    		TAssert(WorkerThreads[Index].TaskGraphWorker->GetThreadId() == Index);
    		return *WorkerThreads[Index].TaskGraphWorker;
    	}

    	ENamedThreads::Type GetCurrentThread()
    	{
    		ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread;
    		FWorkerThread* TLSPointer = (FWorkerThread*)FPlatformTLS::GetTlsValue(PerThreadIdTLSSlot);
    		if (TLSPointer)
    		{
    			TAssert(TLSPointer - WorkerThreads >= 0 && TLSPointer - WorkerThreads < NumThreads);
    			int32 ThreadIndex = static_cast<int32>(TLSPointer - WorkerThreads);
    			TAssert(ThreadFetch(ThreadIndex).GetThreadId() == ThreadIndex);
    			if (ThreadIndex < NumNamedThreads)
    			{
    				CurrentThreadIfKnown = ENamedThreads::Type(ThreadIndex);
    			}
    			else
    			{
    				int32 Priority = (ThreadIndex - NumNamedThreads) / NumTaskThreadsPerSet;
    				CurrentThreadIfKnown = ENamedThreads::SetPriorities(ENamedThreads::Type(ThreadIndex), Priority, false);
    			}
    		}
    		return CurrentThreadIfKnown;
    	}

    private:
        //线程数据
        FWorkerThread        WorkerThreads[65535];
        int32               NumThreads;
        int32				NumNamedThreads;
    	// 线程优先级数量（一个ThreadSet对应的线程数量，范围：1-3）
    	int32				NumTaskThreadSets;
    	// 线程集合（ThreadSet）的数量（不同平台下，根据CPU核心数量计算得出）
    	int32				NumTaskThreadsPerSet;
    	// 控制ThreadSet集合大小，是否创建高/低优先级线程
    	bool                bCreatedHiPriorityThreads;
    	bool                bCreatedBackgroundPriorityThreads;
        /** Index of TLS slot for FWorkerThread* pointer. **/
        uint32				PerThreadIdTLSSlot;

    	/** Array of callbacks to call before shutdown. **/
    	TArray<TFunction<void()> > ShutdownCallbacks;


    	StallingTaskQueue<FBaseGraphTask, PLATFORM_CACHE_LINE_SIZE, 1> IncomingAnyThreadTasks[3]; //numproirities = 1
        
    };

	FBaseGraphTask* FTaskThreadAnyThread::FindWork()
	{
		return TaskGraphImplementationSingleton->FindWork(ThreadId);
	}

    void FTaskGraphInterface::Startup(int32 NumThreads)
    {
        // TaskGraphImplementationSingleton is actually set in the constructor because find work will be called before this returns.
        TaskGraphImplementationSingleton = new TaskGraphImplementation(NumThreads);
    }

    FTaskGraphInterface& FTaskGraphInterface::Get()
    {
        TAssert(TaskGraphImplementationSingleton);
        return *TaskGraphImplementationSingleton;
    }

	FGraphEventRef FGraphEvent::CreateGraphEvent()
    {
    	FGraphEvent* Instance = new (TMemory::Malloc<FGraphEvent>()) FGraphEvent{};
    	return Instance;
    }

    void FGraphEvent::DispatchSubsequents(ENamedThreads::Type CurrentThreadIfKnown)
	{
		TArray<FBaseGraphTask*> NewTasks;
		DispatchSubsequents(NewTasks, CurrentThreadIfKnown);
	}

	void FGraphEvent::DispatchSubsequents(TArray<FBaseGraphTask*>& NewTasks, ENamedThreads::Type CurrentThreadIfKnown, bool bInternal/* = false */)
	{

	}
    
}
#pragma optimize("", on)