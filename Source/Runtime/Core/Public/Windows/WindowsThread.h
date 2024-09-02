#pragma once
#include "HAL/Event.h"
#include "HAL/Thread.h"

namespace Thunder
{
/**
 * Implements the Windows version of the IEvent interface.
 */
class EventWindows : public IEvent
{
public:
	EventWindows() : IEvent(), Event(nullptr), ManualReset(false) {}

	virtual ~EventWindows() override
	{
		if (Event != nullptr)
		{
			CloseHandle(Event);
		}
	}

	virtual bool Create( bool bIsManualReset = false ) override
	{
		// Create the event and default it to non-signaled
		Event = CreateEvent(nullptr, bIsManualReset, 0, nullptr);
		ManualReset = bIsManualReset;

		return Event != nullptr;
	}

	virtual bool IsManualReset() override
	{
		return ManualReset;
	}

	virtual void Trigger() override
	{
		TAssert( Event );
		SetEvent( Event );
	}
	virtual void Reset() override
	{
		TAssert( Event );
		ResetEvent( Event );
	}
	virtual bool Wait( uint32 WaitTime, const bool bIgnoreThreadIdleStats = false ) override;

private:
	/** Holds the handle to the event. */
	HANDLE Event;

	/** Whether the signaled state of the event needs to be reset manually. */
	bool ManualReset;
};
	
 /**
 * This is the base interface for all runnable thread classes. It specifies the
 * methods used in managing its life cycle.
 */
class ThreadWindows : public IThread
{
	/** The thread handle for the thread. */
	HANDLE Thread = 0;

	/**
	 * The thread entry point. Simply forwards the call on to the right
	 * thread main function
	 */
	static ::DWORD __stdcall _ThreadProc(LPVOID pThis)
	{
		TAssert(pThis);
		auto* ThisThread = (ThreadWindows*)pThis;
		ThreadManager::Get().AddThread(ThisThread->GetThreadID(), ThisThread);
		return ThisThread->GuardedRun();
	}

	/** Guarding works only if debugger is not attached or GAlwaysReportCrash is true. */
	uint32 GuardedRun();

	/**
	 * The real thread entry point. It calls the Init/Run/Exit methods on
	 * the runnable object
	 */
	uint32 Run();

public:
	~ThreadWindows()
	{
		if (Thread)
		{
			Kill(true);
		}
	}

	virtual void SetThreadPriority(EThreadPriority NewPriority) override
	{
		ThreadPriority = NewPriority;

		::SetThreadPriority(Thread, TranslateThreadPriority(ThreadPriority));
	}

	virtual void Suspend(bool bShouldPause = true) override
	{
		TAssert(Thread);
		if (bShouldPause == true)
		{
			SuspendThread(Thread);
		}
		else
		{
			ResumeThread(Thread);
		}
	}

	virtual bool Kill(bool bShouldWait = false) override;

	virtual void WaitForCompletion() override
	{
		WaitForSingleObject(Thread, INFINITE);
	}

	static int TranslateThreadPriority(EThreadPriority Priority) {return 1;};
protected:
	virtual bool CreateInternal(ThreadProxy* InRunnable, const String& InThreadName,
		uint32 InStackSize = 0, EThreadPriority InThreadPri = EThreadPriority::Normal) override;
};

}
