#pragma once
#include "Container.h"
#include "Platform.h"
#include "PlatformProcess.h"
#include "HAL/Event.h"
#include "HAL/Thread.h"	
#include "Templates/RefCounting.h"

namespace Thunder
{
/************************ 任务 ************************************/
enum class ETaskPriority : uint8
{
    High,
    Normal,
    Low,
    Num
};
class IWorkInternalData : public RefCountedObject
{
public:
    virtual bool Retract() = 0;
    virtual ~IWorkInternalData() { }
};

//IQueuedWork
class CORE_API ITask
{
public:
    virtual void DoThreadedWork() = 0;
    virtual void Abandon() = 0;
    virtual int64 GetRequiredMemory() const { return -1 /* Negative value means unknown */; }
public:
    virtual ~ITask() { }
    using IInternalDataType = TRefCountPtr<IWorkInternalData>;
    IInternalDataType InternalData;	
};
	
/************************ 线程池 ************************************/
 // FQueueThread 线程池的线程执行体
class ThreadProxy
{
public:
    /** The event that tells the thread there is work to do. */
	IEvent* DoWorkEvent = nullptr;

	/** If true, the thread should exit. */
	std::atomic<bool> TimeToDie { false };

	/** The work this thread is doing. */
	ITask* volatile QueuedWork = nullptr;

	/** The pool this thread belongs to. */
	class FQueuedThreadPoolBase* ThreadPoolOwner = nullptr;

	/** My Thread  */
	IThread* Thread = nullptr;

public:
	bool Init() { return true; }
	void Stop() { }
	void Exit() { }
	uint32 Run();
	bool Create(class FQueuedThreadPoolBase* InPool,uint32 InStackSize = 0, EThreadPriority ThreadPriority = EThreadPriority::Normal);
	bool KillThread();
	void DoWork(ITask* InTask);
};

// FQueuedThreadPool
class CORE_API IThreadPool
{
public:
    virtual bool Create(uint32 InNumQueuedThreads, uint32 StackSize = (32 * 1024), EThreadPriority ThreadPriority = EThreadPriority::Normal, const tchar* Name = nullptr) = 0;
    virtual void Destroy() = 0;
    virtual void AddQueuedWork( ITask* InQueuedWork, ETaskPriority InQueuedWorkPriority = ETaskPriority::Normal) = 0;
    virtual bool RetractQueuedWork(ITask* InQueuedWork) = 0;
    virtual int32 GetNumThreads() const = 0;
public:
    IThreadPool() = default;
    virtual	~IThreadPool() = default;

public:
    static IThreadPool* Allocate();
};


    
}

