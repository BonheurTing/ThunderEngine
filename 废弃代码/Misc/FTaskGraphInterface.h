#pragma once
#pragma optimize("", off)
#include "Container.h"
#include "Container/LockFree.h"
#include "Templates/FunctionMy.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
    namespace ENamedThreads
    {
        enum Type : int32
        {
            UnusedAnchor = -1,
            RHIThread,
            GameThread,   
            RenderThread = GameThread + 1,
            
            AnyThread = 0xff, // "Unknown Thread" or "Any Unnamed Thread"

            ThreadIndexMask = 0xff

            // MainQueue ??
            // NormalTaskPriority ??
        };

        FORCEINLINE Type GetThreadIndex(Type ThreadAndIndex)
        {
            return ((ThreadAndIndex & ThreadIndexMask) == AnyThread) ? AnyThread : Type(ThreadAndIndex & ThreadIndexMask);
        }

    	FORCEINLINE Type SetPriorities(Type ThreadAndIndex, int32 PriorityIndex, bool bHiPri)
        {
        	return Type(ThreadAndIndex);
        }
    }
	
    enum ESubsequentsMode : uint8
    {
    	TrackSubsequents, //另一个任务将依赖于此任务时
		FireAndForget //不依赖其他任务，可节省开销
	};
    
    /** Convenience typedef for a reference counted pointer to a graph event **/
    typedef TRefCountPtr<class FGraphEvent> FGraphEventRef;

    /** Convenience typedef for a an array a graph events **/
    typedef TArray<FGraphEventRef> FGraphEventArray;
    
    /** FTaskGraphInterface **/
    class FTaskGraphInterface
    {
        friend class FBaseGraphTask;

        virtual void PushQueuedTask(class FBaseGraphTask* Task, bool bWakeUpWorker, ENamedThreads::Type ThreadToExecuteOn, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread) = 0;
    public:
        virtual ~FTaskGraphInterface()
        {
        }
        static CORE_API void Startup(int32 NumThreads);
        
        static CORE_API FTaskGraphInterface& Get();

        virtual void AttachToThread(ENamedThreads::Type CurrentThread) = 0;

        //指定线程完成任务列表
        virtual void WaitUntilTasksComplete(const FGraphEventArray& Tasks, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread) = 0;
    	
    	virtual void TriggerEventWhenTasksComplete(IEvent* InEvent, const FGraphEventArray& Tasks, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread, ENamedThreads::Type TriggerThread = ENamedThreads::AnyThread) = 0;

    	//指定线程完成指定任务
        void WaitUntilTaskCompletes(const FGraphEventRef& Task, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread)
        {
            WaitUntilTasksComplete({ Task }, CurrentThreadIfKnown);
        }
        void WaitUntilTaskCompletes(FGraphEventRef&& Task, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread)
        {
            WaitUntilTasksComplete({ std::move(Task) }, CurrentThreadIfKnown);
        }
    	virtual FBaseGraphTask* FindWork(ENamedThreads::Type ThreadInNeed) = 0;
    };

    // Task Graph 执行的任务的基础类，最好都继承自ITask
    // FBaseGraphTask
    class FBaseGraphTask
    {
    public:
    	FBaseGraphTask(int32 InNumberOfPrerequistitesOutstanding)
				: ThreadToExecuteOn(ENamedThreads::AnyThread)
				, NumberOfPrerequisitesOutstanding(InNumberOfPrerequistitesOutstanding + 1) // + 1 is not a prerequisite, it is a lock to prevent it from executing while it is getting prerequisites, one it is safe to execute, call PrerequisitesComplete
    	{
    	}

    	//设置所需的执行线程;
    	void SetThreadToExecuteOn(ENamedThreads::Type InThreadToExecuteOn)
    	{
    		ThreadToExecuteOn = InThreadToExecuteOn;
    	}

    	void PrerequisitesComplete(ENamedThreads::Type CurrentThread, int32 NumAlreadyFinishedPrequistes, bool bUnlock = true)
    	{
    		int32 NumToSub = NumAlreadyFinishedPrequistes + (bUnlock ? 1 : 0); // the +1 is for the "lock" we set up in the constructor
    		if (NumberOfPrerequisitesOutstanding.fetch_sub(NumToSub, std::memory_order_release) == NumToSub) 
    		{
    			bool bWakeUpWorker = true;
    			PushQueuedTask(CurrentThread, bWakeUpWorker);	
    		}
    	}

    	virtual ~FBaseGraphTask()
    	{
    	}

    	void ConditionalQueueTask(ENamedThreads::Type CurrentThread, bool& bWakeUpWorker)
    	{
    		if (NumberOfPrerequisitesOutstanding.fetch_sub(1, std::memory_order_release)==0)
    		{
    			PushQueuedTask(CurrentThread, bWakeUpWorker);
    			bWakeUpWorker = true;
    		}
    	}

    private:
    	friend class NamedTaskThread;
    	friend class FTaskThreadBase;
    	friend class FTaskThreadAnyThread;
    	friend class GraphEvent;
    	friend class TaskGraphImplementation;

    	//实际执行任务。如果bDeleteOnCompletion=1，也将析构并释放旧后端的内存
    	virtual void ExecuteTaskImpl(TArray<FBaseGraphTask*>& NewTasks, ENamedThreads::Type CurrentThread, bool bDeleteOnCompletion) = 0;

    	//新后端实实际删除内存
    	virtual void DeleteTask() = 0;

    	FORCEINLINE void Execute(TArray<FBaseGraphTask*>& NewTasks, ENamedThreads::Type CurrentThread, bool bDeleteOnCompletion)
    	{
    		//可加统计，检验等
    		ExecuteTaskImpl(NewTasks, CurrentThread, bDeleteOnCompletion);
    	}

    	void PushQueuedTask(ENamedThreads::Type CurrentThreadIfKnown, bool bWakeUpWorker)
    	{
    		FTaskGraphInterface::Get().PushQueuedTask(this, bWakeUpWorker, ThreadToExecuteOn, CurrentThreadIfKnown);
    	}
    	
    	ENamedThreads::Type			ThreadToExecuteOn; //要执行的线程
    	std::atomic<int32>			NumberOfPrerequisitesOutstanding; //未完成的先决条件数目。当这个值降为0时，线程将排队等待执行
    };

    //FGraphEvent
    /**
     *	GraphEvent是a list of tasks要等的东西
     *	这些tasks叫做subsequents
     *	GraphEvent是这些subsequents的前置
     *	GraphEvent的生命周期靠引用计数管理
     **/
    class FGraphEvent
    {
    public:
        static CORE_API FGraphEventRef CreateGraphEvent();
        bool AddSubsequent(class FBaseGraphTask* Subsequent)
        {
        	SubsequentList.Push(Subsequent);
        	return true;
        }
        CORE_API void DispatchSubsequents(ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread);
    	CORE_API void DispatchSubsequents(TArray<FBaseGraphTask*>& NewTasks, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread, bool bInternal = false);
        void Wait(ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread)
        {
            FTaskGraphInterface::Get().WaitUntilTaskCompletes(TRefCountPtr(this), CurrentThreadIfKnown);
        }
        ~FGraphEvent()
        {
            TAssert(EventsToWaitFor.empty());
        }

    	bool IsComplete() {return false;}
    private:
        friend class TRefCountPtr<FGraphEvent>;

        // 隐藏构造函数
        FGraphEvent()
        : ThreadToDoGatherOn(ENamedThreads::AnyThread)
        {
        }

        // TRefCountPtr的接口
    public:
        uint32 AddRef()
        {
            const int32 RefCount = ReferenceCount.fetch_add(1, std::memory_order_relaxed);
            TAssert(RefCount > 0);
            return RefCount;
        }

        uint32 Release(bool)
        {
            int32 RefCount = ReferenceCount.fetch_sub(1, std::memory_order_release);
            TAssert(RefCount >= 0);
            if (RefCount == 0)
            {
                //Recycle(this);
            }
            return RefCount;
        }

        uint32 GetRefCount() const
        {
            return ReferenceCount.load();
        }

        static bool NeedAutoMemoryFree() { return false; }
        
    private:
        LockFreeLIFOListBase<FBaseGraphTask, 0>	SubsequentList;
        FGraphEventArray						EventsToWaitFor;
        std::atomic<int32>                      ReferenceCount;
	    ENamedThreads::Type                     ThreadToDoGatherOn;
    };

    /** 
     The user defined task type can take arguments to a constructor. These arguments (unfortunately) must not be references.
     The API required of TTask:
    
    class FGenericTask
    {
        TSomeType	SomeArgument;
    public:
        FGenericTask(TSomeType InSomeArgument) // CAUTION!: Must not use references in the constructor args; use pointers instead if you need by reference
            : SomeArgument(InSomeArgument)
        {
            // Usually the constructor doesn't do anything except save the arguments for use in DoWork or GetDesiredThread.
        }
        ~FGenericTask()
        {
            // you will be destroyed immediately after you execute. Might as well do cleanup in DoWork, but you could also use a destructor.
        }
        FORCEINLINE TStatId GetStatId() const
        {
            RETURN_QUICK_DECLARE_CYCLE_STAT(FGenericTask, STATGROUP_TaskGraphTasks);
        }
    
        [static] ENamedThreads::Type GetDesiredThread()
        {
            return ENamedThreads::[named thread or AnyThread];
        }
        void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
        {
            // The arguments are useful for setting up other tasks. 
            // Do work here, probably using SomeArgument.
            MyCompletionGraphEvent->DontCompleteUntil(TGraphTask<FSomeChildTask>::CreateTask(NULL,CurrentThread).ConstructAndDispatchWhenReady());
        }
    };
    **/

    /** 
	 *	TGraphTask
	 *	Embeds a user defined task, as exemplified above, for doing the work and provides the functionality for setting up and handling prerequisites and subsequents
	 **/
	template<typename TTask>
	class TGraphTask final : public FBaseGraphTask
	{
	public:
		/** 
		 *	This is a helper class returned from the factory. It constructs the embeded task with a set of arguments and sets the task up and makes it ready to execute.
		 *	The task may complete before these routines even return.
		 **/
		class FConstructor
		{
		public:
			/** Passthrough internal task constructor and dispatch. Note! Generally speaking references will not pass through; use pointers */
			template<typename...T>
			FGraphEventRef ConstructAndDispatchWhenReady(T&&... Args)
			{
				new ((void *)&Owner->TaskStorage) TTask(std::forward<T>(Args)...);
				return Owner->Setup(Prerequisites, CurrentThreadIfKnown);
			}

			/** Passthrough internal task constructor and hold. */
			template<typename...T>
			TGraphTask* ConstructAndHold(T&&... Args)
			{
				new ((void *)&Owner->TaskStorage) TTask(std::forward<T>(Args)...);
				return Owner->Hold(Prerequisites, CurrentThreadIfKnown);
			}

		private:
			friend class TGraphTask;

			/** The task that created me to assist with embeded task construction and preparation. */
			TGraphTask*						Owner;
			/** The list of prerequisites. */
			const FGraphEventArray*			Prerequisites;
			/** If known, the current thread.  ENamedThreads::AnyThread is also fine, and if that is the value, we will determine the current thread, as needed, via TLS. */
			ENamedThreads::Type				CurrentThreadIfKnown;

			/** Constructor, simply saves off the arguments for later use after we actually construct the embeded task. */
			FConstructor(TGraphTask* InOwner, const FGraphEventArray* InPrerequisites, ENamedThreads::Type InCurrentThreadIfKnown)
				: Owner(InOwner)
				, Prerequisites(InPrerequisites)
				, CurrentThreadIfKnown(InCurrentThreadIfKnown)
			{
			}
			/** Prohibited copy construction */
			FConstructor(const FConstructor& Other) = delete;
	
			/** Prohibited copy */
			void operator=(const FConstructor& Other) = delete;
		};

		static FConstructor CreateTask(const FGraphEventArray* Prerequisites = NULL, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread)
		{
			int32 NumPrereq = Prerequisites ? Prerequisites->size() : 0;
			return FConstructor(new TGraphTask(TTask::GetSubsequentsMode() == FireAndForget ? NULL : FGraphEvent::CreateGraphEvent(), NumPrereq), Prerequisites, CurrentThreadIfKnown);
		}

		void Unlock(ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread)
		{
			bool bWakeUpWorker = true;
	  		ConditionalQueueTask(CurrentThreadIfKnown, bWakeUpWorker);
		}

		FGraphEventRef GetCompletionEvent()
		{
			return Subsequents;
		}

	private:
		friend class GraphEvent;

		// API derived from BaseGraphTask

		void ExecuteTaskImpl(TArray<FBaseGraphTask*>& NewTasks, ENamedThreads::Type CurrentThread, bool bDeleteOnCompletion) override
		{
			TAssert(false);
		}

		void DeleteTask() final override
		{
			delete this;
		}

		// Internals 
		TGraphTask(FGraphEventRef InSubsequents, int32 NumberOfPrerequistitesOutstanding)
			: FBaseGraphTask(NumberOfPrerequistitesOutstanding)
			, TaskConstructed(false)
		{
			Subsequents.Swap(InSubsequents);
		}
		
		~TGraphTask() override
		{
			TAssert(!TaskConstructed);
		}
		
		void SetupPrereqs(const FGraphEventArray* Prerequisites, ENamedThreads::Type CurrentThreadIfKnown, bool bUnlock)
		{
			TAssert(!TaskConstructed);
			TaskConstructed = true;
			TTask& Task = *(TTask*)&TaskStorage;
			SetThreadToExecuteOn(Task.GetDesiredThread());
			int32 AlreadyCompletedPrerequisites = 0;
			if (Prerequisites)
			{
				for (int32 Index = 0; Index < Prerequisites->size(); Index++)
				{
					FGraphEvent* Prerequisite = (*Prerequisites)[Index];
					if (Prerequisite == nullptr || !Prerequisite->AddSubsequent(this))
					{
						AlreadyCompletedPrerequisites++;
					}
				}
			}
			PrerequisitesComplete(CurrentThreadIfKnown, AlreadyCompletedPrerequisites, bUnlock);
		}
		
		FGraphEventRef Setup(const FGraphEventArray* Prerequisites = NULL, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread)
		{
			FGraphEventRef ReturnedEventRef = Subsequents; // very important so that this doesn't get destroyed before we return
			SetupPrereqs(Prerequisites, CurrentThreadIfKnown, true);
			return ReturnedEventRef;
		}

		TGraphTask* Hold(const FGraphEventArray* Prerequisites = NULL, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread)
		{
			SetupPrereqs(Prerequisites, CurrentThreadIfKnown, false);
			return this;
		}

		static FConstructor CreateTask(FGraphEventRef SubsequentsToAssume, const FGraphEventArray* Prerequisites = NULL, ENamedThreads::Type CurrentThreadIfKnown = ENamedThreads::AnyThread)
		{
			return FConstructor(new TGraphTask(SubsequentsToAssume, Prerequisites ? Prerequisites->size() : 0), Prerequisites, CurrentThreadIfKnown);
		}

		/** An aligned bit of storage to hold the embedded task **/
		TAlignedBytes<sizeof(TTask),alignof(TTask)> TaskStorage;
		/** Used to sanity check the state of the object **/
		bool						TaskConstructed;
		/** A reference counted pointer to the completion event which lists the tasks that have me as a prerequisite. **/
		FGraphEventRef				Subsequents;
	};

	    


    
    // FFunctionGraphTask
    struct FunctionGraphTask
    {
        static FGraphEventRef CreateAndDispatchWhenReady(TFunctionMy<void()> InFunction, const FGraphEventArray* InPrerequisites = nullptr, ENamedThreads::Type InDesiredThread = ENamedThreads::AnyThread)
        {
        	TAssert(false);
        	return nullptr;
            //return TGraphTask<TFunctionGraphTaskImpl<void(), TrackSubsequents>>::CreateTask(InPrerequisites).ConstructAndDispatchWhenReady(MoveTemp(InFunction), InDesiredThread);
        }

        static FGraphEventRef CreateAndDispatchWhenReady(TFunctionMy<void()> InFunction, const FGraphEventRef& InPrerequisite, ENamedThreads::Type InDesiredThread = ENamedThreads::AnyThread)
        {
            FGraphEventArray Prerequisites;
            TAssert(InPrerequisite.Get());
            Prerequisites.push_back(InPrerequisite);
            return CreateAndDispatchWhenReady(MoveTemp(InFunction), &Prerequisites, InDesiredThread);
        }
    };
}
#pragma optimize("", on)